package Net::NfDump;

use 5.000001;
use strict;
use warnings;
use Carp;

require Exporter;
use AutoLoader;
use Socket qw( AF_INET );
use Socket6 qw( inet_ntop inet_pton AF_INET6 );
use Net::NfDump::Fields;

our @ISA = qw(Exporter);

# Items to export into callers namespace by default. Note: do not export
# names by default without a very good reason. Use EXPORT_OK instead.
# Do not simply export all your public functions/methods/constants.

# This allows declaration	use Net::NfDump ':all';
# If you do not need this, moving things directly into @EXPORT or @EXPORT_OK
# will save memory.
our %EXPORT_TAGS = ( 'all' => [ qw(
	ip2txt txt2ip 
	mac2txt txt2mac
	mpls2txt txt2mpls
	flow2txt txt2flow 
	file_info
) ] );

our @EXPORT_OK = ( @{ $EXPORT_TAGS{'all'} } );

our @EXPORT = qw(
	
);

our $VERSION = '0.02_03';

sub AUTOLOAD {
    # This AUTOLOAD is used to 'autoload' constants from the constant()
    # XS function.

    my $constname;
    our $AUTOLOAD;
    ($constname = $AUTOLOAD) =~ s/.*:://;
    croak "&Net::NfDump::constant not defined" if $constname eq 'constant';
    my ($error, $val) = constant($constname);
    if ($error) { croak $error; }
    {
	no strict 'refs';
	# Fixed between 5.005_53 and 5.005_61
#XXX	if ($] >= 5.00561) {
#XXX	    *$AUTOLOAD = sub () { $val };
#XXX	}
#XXX	else {
	    *$AUTOLOAD = sub { $val };
#XXX	}
    }
    goto &$AUTOLOAD;
}

require XSLoader;
XSLoader::load('Net::NfDump', $VERSION);

# Preloaded methods go here.

# Autoload methods go after =cut, and are processed by the autosplit program.

# Below is stub documentation for your module. You'd better edit it!

=head1 NAME

Net::NfDump - Perl API for manipulating with nfdump files

=head1 SYNOPSIS

  use Net::NfDump;
 
  my $src = new Net::NfDump(
        InputFiles => [ 'nfdump_file' ], 
        Filter => 'icmp and src net 10.0.0.0/8',
        Fields => [ '*' ] ); 

  my $dst = new Net::NfDump(
        OutputFile => 'nfdump_icmp_private',
        Fields => [ '*' ] );

  $src->query();

  while (my $row = $src->fetchrow_arrayref() )  {

     $dst->storerow_arrayref($row);

  }

  $src->finish();
  $dst->finish();


=head1 DESCRIPTION


=head1 METHODS

=cut 

# merge $opts with class default opts and return the resilt. 

sub merge_opts {
	my ($self, %opts) = @_;

	my $ropts = {};
	while ( my ($key, $val) =  each %{$self->{opts}} ) {
		$ropts->{$key} = $val;

	}

	while ( my ($key, $val) =  each %opts ) {
		$ropts->{$key} = $val;
	}

	return $ropts; 
}


# Internal function to set output items/fields. At the input takes array that 
# represents string names of the files 
sub set_fields {
	my ($self, @fields) = @_;

	$self->{fields_num} = [];
	$self->{fields_txt} = [];

	foreach (@fields) {

		my $fld = lc($_);

		# add all fields
		if ($fld eq '*') {
			push(@{$self->{fields_num}}, values %Net::NfDump::Fields::NFL_FIELDS_TXT );
			push(@{$self->{fields_txt}}, keys %Net::NfDump::Fields::NFL_FIELDS_TXT );
		# regular item 
		} else {

			if ( !defined($Net::NfDump::Fields::NFL_FIELDS_TXT{$fld}) ) {
				croak("Unknown field \"%s\".", $_); 
			}

			push(@{$self->{fields_num}}, $Net::NfDump::Fields::NFL_FIELDS_TXT{$fld});
			push(@{$self->{fields_txt}}, $fld);;
		}
	}

	$self->{opts}->{Fields} = $self->{fields_txt};

	return Net::NfDump::libnf_set_fields($self->{handle}, $self->{fields_num});
}


=head2 new

The constructor. As the parameter options can be specified. This options will be used
as a default option set in the particular methods. 

Module supports followinng options 

  Options for reading data
  =================================================================
  InputFiles => []   - List of files to  read (arrayref)
  Filter => 'any'    - Filter taht will be applied on input 
                       records. Uses nfdump/tcpdump syntax
  Fields => [ '*' ]  - List of fields to read or update 
                       any field from supported fields can 
                       be used here. See the chapter 
                       "Supported Fields" for the full list 
                       of supported fields. Special field * can 
                       be used for defining all fields. 
  TimeWindowStart => 0
  TimeWindowEnd => 0 - Filter flows that starts or ends in the 
                       specified fied time window. The options uses 
                       unix timestamp values or 0 if the filter shoul
                       not be apllied. 

  Options for writing data
  =================================================================
  OutputFile => undef - Output file for storerow_* methods.
  Compressed => 1    - Flag whether the otput files should be 
                       compressed or not. 
  Anonymized => 0    - Flag indicating that output file contains 
                       anonymized data.
  Ident => ""        - String identificator of files 

=cut

sub new {
	my ($self, %opts) = @_;

	my ($class) = {};
	bless($class);

	my $handle = Net::NfDump::libnf_init();

	if (!defined($handle) || $handle == 0) {
		return undef;
	} 

	$class->{opts} = { 
		InputFiles => [],
		Filter => 'any',
		Fields => [ '*' ],
		TimeWindowStart => 0,
		TimeWindowEnd => 0,
		OutputFile => undef,
		Compressed => 1,
		Anonymized => 0,
		Ident => ""
	};

	$class->{opts} = $class->merge_opts(%opts);
		
	$class->{handle} = $handle;	

	$class->{read_prepared} = 0;
	$class->{write_prepared} = 0;
	$class->{closed} = 0;

	return $class;
}

=head2 info

Returns the information the current state of processing input files. It 
returns information about already processed files, blocks, records. Those
information can be usefull for guessing time of processing whole 
dataset. 

=cut

sub info {
	my ($self) = @_;

	my $ref =  Net::NfDump::libnf_instance_info($self->{handle}); 

	if ($ref->{'total_files'} > 0 && $ref->{'current_total_blocks'} > 0) {
		my $totf = $ref->{'total_files'};
		my $cp = $ref->{'current_processed_blocks'} / $ref->{'current_total_blocks'} * 100;
		$ref->{'percent'} = ($ref->{'processed_files'}  - 1 )/ $totf * 100 + $cp / $totf;
	}

	if (defined($self->{read_started})) {
		my $etime = time() - $self->{read_started};
		$ref->{'elapsed_time'} = $etime;
		$ref->{'remaining_time'} = $etime / ($ref->{'percent'} / 100) - $etime;
	}

	return $ref;

}

=head2 query()

Method that have to be executed before the fetchrow_* method are used. 

=cut 

# Query method can be used in two ways. If the string argument is the 
# flow query is handled. See section FLOW QUERY how to create flow 
# queries.

# =cut

sub query {
	my ($self, %opts) = @_;

	my $o = $self->merge_opts(%opts);

	if (@{$o->{InputFiles}} == 0) {
		croak("No imput files defined");
	} 

	$self->set_fields(@{$o->{Fields}});

	# handle, filter, windows start, windows end, ref to filelist 
	Net::NfDump::libnf_read_files($self->{handle}, $o->{Filter}, 
					$o->{TimeWindowStart}, $o->{TimeWindowEnd}, 
					$o->{InputFiles});	

	$self->{read_prepared} = 1;
	$self->{read_started} = time();

}

=head2 fetchrow_arrayref

Have to be used after query method. If the query wasn't called before the 
method is called as $obj->query() before the first record is returned. 

Method returns array reference with the record and skips to the next record. Returns 
true if there are more records to read or false if all record from all 
files have been read. 

=cut

sub fetchrow_arrayref {
	my ($self) = @_;

	if (!$self->{read_prepared}) {
		$self->query();
	}

	my $ret = Net::NfDump::libnf_read_row($self->{handle});

	#the end of the file/files we set back read prepared to 0
	if (!$ret) {
		$self->{read_prepared} = 0;
	}

	return $ret;
}

=head2 fetchrow_array

Same functionality as fetchrow_arrayref however returns items in array.

=cut 

sub fetchrow_array {
	my ($self) = @_;

	return @{$self->fetchrow_arrayref()};
}

=head2 fetchrow_hashref

Same as fetchrow_arrayref, however the items are returned in the hash reference. 

NOTE: This method can be very uneffective in some cases, please see PERFORMANCE section.

=cut

sub fetchrow_hashref {
	my ($self) = @_;

	my %res;
	my $ref = $self->fetchrow_arrayref();

	return $ref if (!defined($ref));
 
	my $numfields = scalar @{$self->{fields_txt}};	
	for (my $x = 0; $x <  $numfields; $x++) {
		$res{$self->{fields_txt}->[$x]} = $ref->[$x] if defined($ref->[$x]);
	}

	return \%res;
}

=head2 create

Creates a new nfdump file. 

=cut 
sub create {
	my ($self, %opts) = @_;

	my $o = $self->merge_opts(%opts);

	if (!defined($o->{OutputFile}) || $o->{OutputFile} eq "") {
		croak("No output file defined");
	} 

	$self->set_fields(@{$o->{Fields}});

	# handle, filename, compressed, anonyized, identifier 
	Net::NfDump::libnf_create_file($self->{handle}, 
		$o->{OutputFile}, 
		$o->{Compressed},
		$o->{Anonymized},
		$o->{Ident});

	$self->{write_prepared} = 1;
}

=head2 storerow_arrayref

Insert data defined in arrayref to the file opened by create.  The number of 
fields and their order have to respect order defined in the Fileds option 
handled during $obj->new() or $obj->create() method. 

=cut

sub storerow_arrayref {
	my ($self, $row) = @_;

	if (!$self->{write_prepared}) {
		$self->create();
	}

	return Net::NfDump::libnf_write_row($self->{handle}, $row);
}

=head2 storerow_array 

Same as storerow_arrayref, however items are handled as the single array 

=cut

sub storerow_array {
	my ($self, @row) = @_;

	return $self->storerow_arrayref(\@row);
}

=head2 storerow_hashref

Inserts structure defined as hash reference into output file. 

NOTE: This method can be very uneffective in some cases, please see PERFORMANCE section.

=cut

sub storerow_hashref {
	my ($self, $row) = @_;

	return undef if (!defined($row));

	$self->set_fields( keys %{$row} );

	return $self->storerow_array( values %{$row} );
	
}

=head2 clonerow

Copy the full content of the row from the source object (instance). This method 
is usefull for writing effective scripts (it's much faster that any of the
prevous row).

=cut

sub clonerow {
	my ($self, $obj) = @_;

	return undef if ( !defined($obj) || !defined($obj->{handle}) );

	if (!$self->{write_prepared}) {
		$self->create();
	}

	return Net::NfDump::libnf_copy_row($self->{handle}, $obj->{handle});
}

=head2 finish

Closes all openes file handles. It is nescessary to call that method specilly 
when a new file is created. The method flushes to file records that remains in the memory 
buffer and updates file statistics in the header. Withat calling this method the 
output file might be corupted. 

=cut

sub finish {
	my ($self) = @_;

	# handle, row reference
	Net::NfDump::libnf_finish($self->{handle});

	$self->{read_prepared} = 0;
	$self->{write_prepared} = 0;
	$self->{closed} = 1;
}

sub DESTROY {
	my ($self) = @_;

	# handle, row reference
	if (!$self->{closed}) {
		Net::NfDump::libnf_finish($self->{handle});
	}
}


=head1 FLOW QUERY - NOT IMPLEMENTED YET

The flow query is language vyry simmilar to SQL to query data on 
nfdump files. However flow query have nothing to do with SQL. It uses
only simmilar command syntax. Example of flow query 

  SELECT * FROM data/nfdump1.nfcap, data2/nfdump2.nfcap
  WHERE src host 147.229.3.10 
  TIME WINDOW BETWEEN '2012-06-03' AND '202-06-04' 
  ORDER BY bytes
  LIMIT 100


  INSERT INTO data/nout_nfdump.nfcap (srcip, dstip, srcport, dstport) 

=head1 NOTE ABOUT 32BIT PLATFORMS

Nfdump primary uses 64 bit counters and other items to store single integer value. However 
the native 64 bit support is not compiled in every perl. For thoose cases where 
only 32 integer values are supported the Net::NfDump uses Math::Int64 module. 

The build scripts automatically detect the platform and Math::Int64 module is required
only on platforms where perl do not supports 64bit integer values. 

=head1 EXTRA CONVERTION FUNCTIONS 

The module also provides extra convertion functions that allow convert binnary format 
of IP address, MAC address and MPLS labels tag into text format and back. 

Those functions are not exported by default 

=head2 ip2txt 

Converts both IPv4 and IPv6 address into text form. The standart inet_ntop function 
can be used instead to provide same results. 

=cut 

sub ip2txt ($) {
	my ($addr) = @_;

	if (!defined($addr)) {
		return undef;
	}

	my $type;

	if (length($addr) == 4) {
		$type = AF_INET;
	} elsif (length($addr) == 16) {
		$type = AF_INET6;
	} else {
		carp("Invalid IP address length in binnary representation");
		return undef;
	}

	return inet_ntop($type, $addr);
}

=pod

=head2 txt2ip 

Inversion fuction to ip2txt. Returns binnary format of IP addres or undef 
if the conversion is impossible. 

=cut 

sub txt2ip ($) {
	my ($addr) = @_;
	my $type;

	if (!defined($addr)) {
		return undef;
	}

	if (index($addr, ':') != -1) {
		$type = AF_INET6;
	} else {
		$type = AF_INET;
	}

    return inet_pton($type, $addr);
}

=pod

=head2 mac2txt 

Converts MAC addres to xx:yy:xx:yy:xx:yy format. 

=cut 

sub mac2txt ($) {
	my ($addr) = @_;

	if (!defined($addr)) {
		return undef;
	}

	if (length($addr) != 6) {
		carp("Invalid MAC address length in binnary representation");
		return undef;
	}

	return sprintf("%s%s:%s%s:%s%s:%s%s:%s%s:%s%s", split('',unpack("H12", $addr)));
}

=pod

=head2 txt2mac 

Inversion fuction to mac2txt. Accept address in any of following format 
aabbccddeeff
aa:bb:cc:dd:ee:ff
aa-bb-cc-dd-ee-ff
aabb-ccdd-eeff

Return the binnary format of the address or undef if confersion is impossible. 

=cut 

sub txt2mac ($) {
	my ($addr) = @_;

	if (!defined($addr)) {
		return undef;
	}

	$addr =~ s/[\-|\:]//g;

	if (length($addr) != 12) {
		return undef;
	}

	return pack('H12', $addr);
}

=pod

=head2 mpls2txt 

Converts label information to format B<Lbl-Exp-S>

Whwre 
Lbl - Value given to the MPLS label by the router. 
Exp - Value of experimental bit. 
S - Value of the end-of-stack bit: Set to 1 for the oldest entry in the stack and to zero for all other entries. 

=cut 

sub mpls2txt ($) {
	my ($addr) = @_;

	if (!defined($addr)) {
		return undef;
	}
	my @res;

	foreach (unpack('I*', $addr)) {

		my $lbl = $_ >> 12;
		my $exp = ($_ >> 9 ) & 0x7;
		my $eos = ($_ >> 8 ) & 0x1;

		push(@res, sprintf "%d-%d-%d", $lbl, $exp, $eos) if ($_ != 0);
	}

	return  join(' ', @res);
}

=pod

=head2 txt2mpls

Inversion function to mpls2txt. As the argiment expects the text representaion 
of the MPLS labels as was described in the previous function (B<Lbl-Exp-S>)

=cut 

sub txt2mpls ($) {
	my ($addr) = @_;

	if (!defined($addr)) {
		return undef;
	}

	my $res =  "";

	my @labels = split(/\s+/, $addr); 

	foreach (@labels) {
		my ($lbl, $exp, $eos) = split(/\-/);

		my $label = ($lbl << 12) | ( $exp << 9 ) | ($eos << 8 ); 
		
		$res .= pack("I", $label);	
	}


	$res .= pack("I", 0x0) x (10 - length($res) / 4);	# alogn to 10 items (4 * 10 Bytes) 
	return  $res;
}

=pod

=head2 flow2txt

Gets hash reference to items returned by fetchrow_hashref and converts all items into
human readable text format. Applies finction ip2txt, mac2txt, mpl2txt to the items 
where it make sense. 

=cut 

# how to convert particular type to 
my %CVTTYPE = ( 
	'srcip' => 'ip', 'dstip' => 'ip', 'nexthop' => 'ip', 'bgpnexthop' => 'ip', 'router' => 'ip',
	'insrcmac' => 'mac', 'outsrcmac' => 'mac', 'indstmac' => 'mac', 'outdstmac' => 'mac',
	'mpls' => 'mpls' );
	

sub flow2txt ($) {
	my ($row) = @_;
	my %res;
	
	while ( my ($key, $val) = each %{$row}) {

		if ( defined($CVTTYPE{$key}) ) { 
			my $cvt = $CVTTYPE{$key};
			if ($cvt eq 'ip') {
				$res{$key} = ip2txt($val);
			} elsif ($cvt eq 'mac') {
				$res{$key} = mac2txt($val);
			} elsif ($cvt eq 'mpls') {
				$res{$key} = mpls2txt($val);
			} else {
				croak("Invalid conversion type $cvt");
			}
		} else {
			$res{$key} = $val;
		}
	}

	return \%res;	
}

=pod

=head2 txt2flow

Inversion function to flow2txt. It is usefull before calling storerow_hashref

=cut 


sub txt2flow ($) {
	my ($row) = @_;
	my %res;
	
	while ( my ($key, $val) = each %{$row}) {

		if ( defined($CVTTYPE{$key}) ) { 
			my $cvt = $CVTTYPE{$key};
			if ($cvt eq 'ip') {
				$res{$key} = txt2ip($val);
			} elsif ($cvt eq 'mac') {
				$res{$key} = txt2mac($val);
			} elsif ($cvt eq 'mpls') {
				$res{$key} = txt2mpls($val);
			} else {
				croak("Invalid conversion type $cvt");
			}
		} else {
			$res{$key} = $val;
		}
	}

	return \%res;	
}

=pod 

=head2 file_info

Reads information from nfdump file header. It provides various atributes 
like number of blocks, version, flags, statistics, etc.  As the result the 
follwing items are returned: 

  version
  ident
  blocks
  catalog
  anonymized
  compressed
  sequence_failures

  first
  msec_first
  last
  msec_last

  flows, bytes, packets

  flows_tcp, flows_udp, flows_icmp, flows_other
  bytes_tcp, bytes_udp, bytes_icmp, bytes_other
  packets_tcp, packets_udp, packets_icmp, packets_other

=cut

sub file_info {
	my ($file) = @_;

	my $ref =  Net::NfDump::libnf_file_info($file); 

	return $ref;
}


=head1 SUPPORTED ITEMS 

  Time items
  =====================
  first - Timestamp of first seen packet 
  msecfirst - Number of miliseconds of first seen packet since B<first>  
  last - Timestamp of last seen packet 
  mseclast - Number of miliseconds of last seen packet since B<last>  
  received - Timestamp when the packet was received by collector 

  Statistical items
  =====================
  bytes - The number of bytes 
  pkts - The number of packets 
  outbytes - The number of output bytes 
  outpkts - The number of output packets 
  flows - The number of flows (aggregated) 

  Layer 4 information
  =====================
  srcport - Source port 
  dstport - Destination port 
  tcpflags - TCP flags  

  Layer 3 information
  =====================
  srcip - Source IP address 
  dstip - Destination IP address 
  nexthop - IP next hop 
  srcmask - Source mask 
  dstmask - Destination mask 
  tos - Source type of service 
  dsttos - Destination type of Service 
  srcas - Source AS number 
  dstas - Destination AS number 
  nextas - BGP Next AS 
  prevas - BGP Previous AS 
  bgpnexthop - BGP next hop 
  proto - IP protocol  

  Layer 2 information
  =====================
  srcvlan - Source vlan label 
  dstvlan - Destination vlan label 
  insrcmac - In source MAC address 
  outsrcmac - Out destination MAC address 
  indstmac - In destintation MAC address 
  outdstmac - Out source MAC address 

  MPLS information
  =====================
  mpls - MPLS labels 

  Layer 1 information
  =====================
  inif - SNMP input interface number 
  outif - SNMP output interface number 
  dir - Flow directions ingress/egress 
  fwd - Forwarding status 

  Exporter information
  =====================
  router - Exporting router IP 
  systype - Type of exporter 
  sysid - Internal SysID of exporter 

  Extra/special fields
  =====================
  clientdelay - nprobe latency client_nw_delay_usec 
  serverdelay - nprobe latency server_nw_delay_usec
  appllatency - nprobe latency appl_latency_usec

=head1 SEE ALSO

http://nfdump.sourceforge.net/

=head1 AUTHOR

Tomas Podermanski, E<lt>tpoder@cis.vutbr.czE<gt>, Brno University of Technology

=head1 COPYRIGHT AND LICENSE

Copyright (C) 2012 by Brno University of Technology

This library is free software; you can redistribute it and/or modify
it under the same terms as Perl itself.


=cut

1;

__END__

