package Net::NfDump;

use 5.000001;
use strict;
use warnings;
use Carp;

require Exporter;
use AutoLoader;
use Socket qw( AF_INET );
use Socket6 qw( inet_ntop inet_pton AF_INET6 );

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
	row2txt txt2row 
) ] );

our @EXPORT_OK = ( @{ $EXPORT_TAGS{'all'} } );

our @EXPORT = qw(
	
);

our $VERSION = '0.01_01';

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

Net::NfDump - Perl extension for blah blah blah

=head1 SYNOPSIS

  use Net::NfDump;
  blah blah blah

=head1 DESCRIPTION

Stub documentation for Net::NfDump, created by h2xs. It looks like the
author of the extension was negligent enough to leave the stub
unedited.

Blah blah blah.

=head2 EXPORT

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


=head2 new
The constructor. As the parameter options can be specified. This options will be used
as a default option set in the particular methods. 

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
		Filter => "any",
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

=head2 file_info
Reads information from nfdump file header. It provides various atributes 
like number of blocks, version, flags, statistics, etc.  related to the file. 
Return has hreference with items

=cut

=head2 info
Returns the information the current state of processing input files. It 
returns information about already processed files, blocks, records. Those
information can be usefull for guessing time of processing whole 
dataset. 

=cut

sub info {
	my ($self, $file) = @_;

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


=head2 query
Query method can be used in two ways. If the string argument is the 
flow query is handled. See section FLOW QUERY how to create flow 
queries.

=cut

sub query {
	my ($self, %opts) = @_;

	my $o = $self->merge_opts(%opts);

	if (@{$o->{InputFiles}} == 0) {
		croak("No imput files defined");
	} 

	# handle, filter, windows start, windows end, ref to filelist 
	Net::NfDump::libnf_read_files($self->{handle}, $o->{Filter}, 
					$o->{TimeWindowStart}, $o->{TimeWindowEnd}, 
					$o->{InputFiles});	

	$self->{read_prepared} = 1;
	$self->{read_started} = time();

}

=head2 read
Have to be used after query method. If the query wasn't called before the 
method is called as $obj->query() before the first record is returned. 

Method returns hash reference with the record and skips to the next record. Returns 
true if there are more records to read or false if all record from all 
files have been read. 

=cut

sub fetchrow_hashref {
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

sub fetchrow_arrayref {
	my ($self) = @_;

	croak("Not implemented yet. Reserver for future use.");
}

sub fetchrow_array {
	my ($self) = @_;

	croak("Not implemented yet. Reserver for future use.");
}

=head2 create_file
Creates a new nfdump file. 

=cut 
sub create {
	my ($self, %opts) = @_;

	my $o = $self->merge_opts(%opts);

	if (!defined($o->{OutputFile}) || $o->{OutputFile} eq "") {
		croak("No output file defined");
	} 

	# handle, filename, compressed, anonyized, identifier 
	Net::NfDump::libnf_create_file($self->{handle}, 
		$o->{OutputFile}, 
		$o->{Compressed},
		$o->{Anonymized},
		$o->{Ident});

	$self->{write_prepared} = 1;
}


=head2 storerow_hashref
Insert data defined in hashref to the file opened by create. 

=cut

sub storerow_hashref {
	my ($self, $row) = @_;

	if (!$self->{write_prepared}) {
		$self->create();
	}
	
	# handle, row reference
	return Net::NfDump::libnf_write_row($self->{handle}, $row);
}

sub storerow_arryref {
	my ($self, $row) = @_;

	croak("Not implemented yet. Reserver for future use.");
}

sub storerow_array {
	my ($self, $row) = @_;

	croak("Not implemented yet. Reserver for future use.");
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

=head2 row2txt
Gets hash reference to items returned by fetchrow_hashref and converts all items into
human readable text format. Applies finction ip2txt, mac2txt, mpl2txt to the items 
where it make sense. 

=cut 

# how to convert particular type to 
my %CVTTYPE = ( 
	'srcip' => 'ip', 'dstip' => 'ip', 'nexthop' => 'ip', 'bgpnexthop' => 'ip', 'router' => 'ip',
	'insrcmac' => 'mac', 'outsrcmac' => 'mac', 'indstmac' => 'mac', 'outdstmac' => 'mac',
	'mpls' => 'mpls' );
	

sub row2txt ($) {
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

=head2 txt2row
Inversion function to row2txt. It is usefull before calling storerow_hashref

=cut 


sub txt2row ($) {
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
=head1 SUPPORTED ITEMS 

 Flow received time in ms
=item srcport Source port
 Destination port
 TCP flags 
 Source IP address
 Destination IP address
 IP next hop
 Source mask
 Destination mask
 Source type of service
 Destination type of Service
 Source AS number
 Destination AS number
 BGP Next AS
 BGP Previous AS
 BGP next hop
 IP protocol 
 Source vlan label
 Destination vlan label
 In source MAC address
 Out destination MAC address
 In destintation MAC address
 Out source MAC address
 MPLS label
 SNMP input interface number
 SNMP output interface number
 Flow directions ingress/egress
 Forwarding status
 Exporting router IP
 Type of exporter
 Internal SysID of exporter
 nprobe latency client_nw_delay_usec
 nprobe latency server_nw_delay_usec
 nprobe latency appl_latency_usec
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

