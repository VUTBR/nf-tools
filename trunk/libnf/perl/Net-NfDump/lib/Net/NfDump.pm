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

our $VERSION = '0.05';

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

  #
  #
  # Example 1: reading nfdump file(s)
  # 
  
  $flow = new Net::NfDump(
              InputFiles => [ 'nfdump_file1', 'nfdump_file2' ], 
              Filter => 'icmp and src net 10.0.0.0/8',
              Fields => 'proto, bytes' ); 

  $flow->query();

  while (my ($proto, $bytes) = $flow->fetchrow_array() )  {
      $h{$proto} += $bytes;
  }
  $flow->finish();

  foreach ( keys %h ) {
      printf "%s %d\n", $_, $h{$_};
  }


  #
  #
  # Example 2: creating and writing records into nfdump file
  #
  
  $flow = new Net::NfDump(
              OutputFile => 'output.nfcap',
              Fields => 'srcip,dstip' );

  $flow->storerow_arrayref( [ txt2ip('147.229.3.10'), txt2ip('1.2.3.4') ] );

  $flow->finish();


  #
  #
  # Example 3: reading/writing (merging two input files) and swap
  #            source and destination address if the destination port 
  #            is 80/http (I know it doesn't make much sense).
  #

  $flow1 = new Net::NfDump( 
               InputFiles => [ 'nfdump_file1', 'nfdump_file2' ], 
               Fields => 'srcip, dstip, dstport' ); 

  $flow2 = new Net::NfDump( 
               OutputFile => 'nfdump_file_out', 
               Fields => 'srcip, dstip, dstport' ); 

  $flow1->query();
  $flow2->create();

  while (my $ref = $flow->fetchrow_arrayref() )  {

      if ( $ref->[2] == 80 ) { 
          ($ref->[0], $ref->[1]) = ($ref->[1], $ref->[0]);
      }

     $flow2->clonerow($flow1);
     $flow2->storerow_arrayref($ref);

  }

  $flow1->finish();
  $flow2->finish();


=head1 DESCRIPTION

Nfdump L<http://nfdump.sourceforge.net/> is very polpular toolset 
for collecting, storing and processing NetFlow/SFlow/IPFIX data. The 
one of the key tool is command line utility bearing the same name
as whole toolset (nfdump). Although this utility can process data 
very speed, it is cumbersome for some apllications. 

This module implements basic operations on binary files produced
with nfdump tool. It allows read, create and write flow records on
thoose files.  The modules tries to keep naming conventions for 
methods same as are used in DBI nodules/API, so developers that 
are used to use this interface should be famillar with the 
interface. 

The module uses original nfdump sources to implement nescessary 
functions. The compatibility with the original 
nfdump should be eaisly keept and there should be a minimal effort
to cope with future version of the nfdump tool. 

The architecture is following: 

      
          APPLICATION 
   +------------------------+
   |                        |  Implements all methods and functions 
   | Net::NfDump API (perl) |  described in this document.
   |                        |
   +------------------------+
   |                        |  The code converting internal nfdump 
   | libnf - glue code (C)  |  structures into perl and back to C.
   |                        |
   +------------------------+
   |                        |  All original nfdump source files. There  
   |   nfdump sources (C)   |  are no changes in theese files and all  
   |                        |  changes are placed into libnf code.
   +------------------------+  
         NFDUMP FILES


This version of Net::NfDump module is based on B<nfdump-1.6.9> available on L<http://sourceforge.net/projects/nfdump/>. Support for NSEL and NEL code is enabled. 


=cut 

# converts comma seperated string to array reference 
sub split_str($) {
	my ($arg) = @_;

	if (ref $arg eq 'ARRAY') {
		# already is an array
		return $arg;
	} else {
		my @arr = split(/,\s*/, $arg);
		chomp @arr;
		return \@arr;
	}

}


# merge $opts with class default opts and return the resilt. 

sub merge_opts {
	my ($self, %opts) = @_;

	my $ropts = {};
	while ( my ($key, $val) =  each %{$self->{opts}} ) {
		$ropts->{$key} = $val;

	}

	while ( my ($key, $val) =  each %opts ) {
		if ($key eq "InputFiles" || $key eq "Fields") {
			$val = split_str($val);
		}
		$ropts->{$key} = $val;
	}

	return $ropts; 
}

# Internal function to set output items/fields. At the input takes array that 
# represents string names of the files 
sub set_fields {
	my ($self, $fieldsref) = @_;

	$self->{fields_num} = [];
	$self->{fields_txt} = [];

	foreach (@{$fieldsref}) {

		my $fld = lc($_);

		# add all fields
		if ($fld eq '*') {
			push(@{$self->{fields_num}}, values %Net::NfDump::Fields::NFL_FIELDS_TXT );
			push(@{$self->{fields_txt}}, keys %Net::NfDump::Fields::NFL_FIELDS_TXT );
		# regular item 
		} else {

			if ( !defined($Net::NfDump::Fields::NFL_FIELDS_TXT{$fld}) ) {
				croak(sprintf("Unknown field \"%s\".", $_)); 
			}

			push(@{$self->{fields_num}}, $Net::NfDump::Fields::NFL_FIELDS_TXT{$fld});
			push(@{$self->{fields_txt}}, $fld);;
		}
	}

	$self->{opts}->{Fields} = $self->{fields_txt};

	return Net::NfDump::libnf_set_fields($self->{handle}, $self->{fields_num});
}



=head1 METHODS, OPTIONS AND RELATED FUNCTIONS

=head2 Options

Options can be nahdled in varios methods. The basic options ses can be handled 
in the constructor and than modified in methods like $obj->query() or $obj->create(). 

The values after => indicates the default value for the item.


=over 

=item * B<InputFiles> => []

List of files to read (arrayref).  

=item * B<Filter> => 'any'

Filter taht will be applied on input records. Uses nfdump/tcpdump syntax. 

=item * B<Fields> => '*'

List of fields to read or update any field from supported fields can be used 
here. See the chapter "Supported Fields" for the full list of supported 
fields.  Special field * can be used for defining all fields. 


=item * B<TimeWindowStart>, B<TimeWindowEnd> => 0

Filter flows that starts or ends in the specified fied time window. 
The options uses unix timestamp values or 0 if the filter should
not be apllied. 

=item *  B<OutputFile> => undef

Output file for storerow_* methods. Default: undef

=item * B<Compressed> => 1

Flag whether the otput files should be compressed or not. 

=item * B<Anonymized> => 0

Flag indicating that output file contains anonymized data.

=item * B<Ident> => '' 

String identificator of files stored into the header of the file. 

=back 

=head2 Constructor, status informations methods

=over 

=item * B<$obj = new Net::NfDump( %opts )>


  my $obj = new Net::NfDump( InputFiles => [ 'file1']  );


The constructor. As the parameter options can be specified. 


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
	$class->{last_hashref_items} = "";

	return $class;
}

=pod

=item * B<< $ref = $obj->info() >>


  my $i = $obj->info();
  print Dumper($i);


Returns the information the current state of processing input files. It 
returns information about already processed files, blocks, records. Those
information can be usefull for guessing time of processing whole 
dataset. Hashref returs following items:

  total_files           - total number of files to process
  elapsed_time          - elapsed time 
  remaining_time        - guessed remaining time to process all records
  percent               - guessed percent of processed records
  
  processed_files       - total number of processed files
  processed_records     - total number of processed records
  processed_blocks      - total number of processed blocks
  processed_bytes       - total number of processed bytes 
                          number of bytes read from file 
                          system after uncompressing 
  
  current_filename      - the name of the file currently processed
  current_total_blocks  - the number of blocks in the currently 
                          processed file 
  current_processed_blocks -  the number of processd blocks in the 
                          currently processed file

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


=pod

=item * B<< $obj->finish() >>


  $obj->finish();


Closes all openes file handles. It is nescessary to call that method specilly 
when a new file is created. The method flushes to file records that remains in the memory 
buffer and updates file statistics in the header. Withat calling this method the 
output file might be corupted. 

=back

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

=pod

=head2 Methods for reading data 

=over 

=item * B<< $obj->query( %opts ) >>


  $obj->query( Filter => 'src host 10.10.10.1' );


Method that have to be executed before any of the C<fetchrow_*> method  is used. Options 
can be handled to the method. 

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

	$self->set_fields($o->{Fields});

	# handle, filter, windows start, windows end, ref to filelist 
	Net::NfDump::libnf_read_files($self->{handle}, $o->{Filter}, 
					$o->{TimeWindowStart}, $o->{TimeWindowEnd}, 
					$o->{InputFiles});	

	$self->{read_prepared} = 1;
	$self->{read_started} = time();

}

=pod 

=item * B<< $ref = $obj->fetchrow_arrayref() >>


  while (my $ref = $obj->fetchrow_arrayref() ) {
      print Dumper($ref);
  }


Have to be used after query method. The method $obj->query() s called 
automatically if it wasn't called before. 

Method returns array reference with the record and skips to the next record. Returns 
true if there are more records to read or undef if end of the record set have been reached. 

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

=pod 

=item * B<< @array = $obj->fetchrow_array() >>


  while ( @array = $obj->fetchrow_arrayref() ) { 
    print Dumper(\@array);
  }


Same functionality as fetchrow_arrayref however returns items in array instead.

=cut 

sub fetchrow_array {
	my ($self) = @_;

	my $ref = $self->fetchrow_arrayref();

	return if (!defined($ref));

	return @{$ref};
}

=pod 

=item * B<< $ref = $obj->fetchrow_hashref() >>


  while ( $ref = $obj->fetchrow_hashref() ) {
     print Dumper($ref);
  }


Same as fetchrow_arrayref, however the items are returned in the hash reference as the 
key => vallue tuples. 

NOTE: This method can be very uneffective in some cases, please see PERFORMANCE section.

=back 

=cut

sub fetchrow_hashref {
	my ($self) = @_;

	my %res;
	my $ref = $self->fetchrow_arrayref();

	return if (!defined($ref));
 
	my $numfields = scalar @{$self->{fields_txt}};	
	for (my $x = 0; $x <  $numfields; $x++) {
		$res{$self->{fields_txt}->[$x]} = $ref->[$x] if defined($ref->[$x]);
	}

	return \%res;
}

=pod

=head2 Methods for writing data 

=over 

=item * B<< $obj->create( %opts ) >>


  $obj->create( OutputFile => 'output.nfcapd' );


Creates a new nfdump file. This method have to be called before any of $obj->storerow_* 
method is called. 

=cut 

sub create {
	my ($self, %opts) = @_;

	my $o = $self->merge_opts(%opts);

	if (!defined($o->{OutputFile}) || $o->{OutputFile} eq "") {
		croak("No output file defined");
	} 

	$self->set_fields($o->{Fields});

	# handle, filename, compressed, anonyized, identifier 
	Net::NfDump::libnf_create_file($self->{handle}, 
		$o->{OutputFile}, 
		$o->{Compressed},
		$o->{Anonymized},
		$o->{Ident});

	$self->{write_prepared} = 1;
}

=pod 

=item * B<< $obj->storerow_arrayref( [ @array ] ) >>


  $obj->storerow_arrayref( [ $srcip, $dstip ] );


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

=pod

=item * B<< $obj->storerow_array( @array ) >>


  $obj->storerow_array( $srcip, $dstip );


Same as storerow_arrayref, however items are handled as the single array 

=cut

sub storerow_array {
	my ($self, @row) = @_;

	return $self->storerow_arrayref(\@row);
}

=pod

=item * B<< $obj->storerow_hashref ( \%hash ) >>


  $obj->storerow_hashref( { 'srcip' =>  $srcip, 'dstip' => $dstip } );


Inserts structure defined as hash reference into output file. 

NOTE: This method can be very uneffective in some cases, please see PERFORMANCE section.

=cut

sub storerow_hashref {
	my ($self, $row) = @_;

	return undef if (!defined($row));

	if (join(',', keys %{$row}) ne $self->{last_hashref_items}) {
		$self->set_fields( [ keys %{$row} ] );
		$self->{last_hashref_items} = join(',', keys %{$row});
	}

	return $self->storerow_arrayref( [ values %{$row} ] );
	
}

=pod

=item * B<< $obj->clonerow( $obj2 ) >>


  $obj->clonerow( $obj2 );


Copy the full content of the row from the source object (instance). This method 
is usefull for writing effective scripts (it's much faster that any of the
prevous row).

=back

=cut

sub clonerow {
	my ($self, $obj) = @_;

	return undef if ( !defined($obj) || !defined($obj->{handle}) );

	if (!$self->{write_prepared}) {
		$self->create();
	}

	return Net::NfDump::libnf_copy_row($self->{handle}, $obj->{handle});
}

=pod

=head2 Extra conversion and support functions

The module also provides extra convertion functions that allow convert binnary format 
of IP address, MAC address and MPLS labels tag into text format and back. 

Those functions are not exported by default, so it have to be either called 
with full module name or imported when the module is loades. For importing 
all support function C<:all> synonym can be used. 
 
  use Net::NfDump qw ':all';

=over 

=item * B<< $txt = ip2txt( $bin ) >>

=item * B<< $bin = txt2ip( $txt ) >>


  $ip = txt2ip('10.10.10.1');
  print ip2txt($ip);


Converts both IPv4 and IPv6 address into text form and back. The standart 
inet_ntop/inet_pton functions can be used instead to provide same results. 

Function txt2ip returns binnary format of IP addres or undef 
if the conversion is impossible. 

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

=item * B<< $txt = mac2txt( $bin ) >>

=item * B<< $bin = txt2mac( $txt ) >>


  $mac = txt2mac('aa:02:c2:2d:e0:12');
  print mac2txt($mac);


Converts MAC addres to xx:yy:xx:yy:xx:yy format and back. The fuction to mac2txt 
accepts an address in any of following format: 

  aabbccddeeff
  aa:bb:cc:dd:ee:ff
  aa-bb-cc-dd-ee-ff
  aabb-ccdd-eeff

Returns the binnary format of the address or undef if confersion is impossible. 

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

=item * B<< $txt = mpls2txt( $mpls ) >>

=item * B<< $mpls = txt2mpls( $txt ) >>


  $mpls = txt2mpls('1002-6-0 1003-6-0 1004-0-1');
  print mpls2txt($mpls);


Converts label information to format B<Lbl-Exp-S> and back. 

Where:
 
  Lbl - Value given to the MPLS label by the router. 
  Exp - Value of experimental bit. 
  S   - Value of the end-of-stack bit: Set to 1 for the oldest 
        entry in the stack and to zero for all other entries. 

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

=item * B<< $ref = flow2txt( \%row ) >>

=item * B<< $ref = txt2flow( \%row ) >>


The function flow2txt gets hash reference to items returned by fetchrow_hashref and 
converts all items into humman readable text format. Applies finctions 
ip2txt, mac2txt, mpl2txt to the items where it make sense.  The function 
txt2flow does opossite functionality.

=cut 

# how to convert particular type to 
my %CVTTYPE = ( 
	'srcip' => 'ip', 'dstip' => 'ip', 'nexthop' => 'ip', 'bgpnexthop' => 'ip', 'router' => 'ip',
	'insrcmac' => 'mac', 'outsrcmac' => 'mac', 'indstmac' => 'mac', 'outdstmac' => 'mac',
	'mpls' => 'mpls',
	'xsrcip' => 'ip', 'xdstip' => 'ip', 'nsrcip' => 'ip', 'ndstip' => 'ip' );
	

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

=item * B<< $ref = file_info( $file_name ) >>


  $ref = file_info('file.nfcap');
  print Dumper($ref);


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
  last

  flows, bytes, packets

  flows_tcp, flows_udp, flows_icmp, flows_other
  bytes_tcp, bytes_udp, bytes_icmp, bytes_other
  packets_tcp, packets_udp, packets_icmp, packets_other


=back 

=cut

sub file_info {
	my ($file) = @_;

	my $ref =  Net::NfDump::libnf_file_info($file); 

	return $ref;
}

=pod


=head1 SUPPORTED ITEMS 

  Time items
  =====================
  first - Timestamp of first seen packet in miliseconds
  last - Timestamp of last seen packet in miliseconds
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

  NSEL fields, see: http://www.cisco.com/en/US/docs/security/asa/asa81/netflow/netflow.html
  =====================
  flowstart - NSEL The time that the flow was create
  connid - NSEL An identifier of a unique flow for the device 
  icmpcode - NSEL ICMP code value 
  icmptype - NSEL ICMP type value 
  event - NSEL High-level event code
  xevent - NSEL Extended event code
  xsrcip - NSEL Mapped source IPv4 address 
  xdstip - NSEL Mapped destination IPv4 address 
  xsrcport - NSEL Mapped source port 
  xdstport - NSEL Mapped destination port 
 NSEL The input ACL that permitted or denied the flow
  iacl - Hash value or ID of the ACL name
  iace - Hash value or ID of the ACL name 
  ixace - Hash value or ID of an extended ACE configuration 
 NSEL The output ACL that permitted or denied a flow  
  eacl - Hash value or ID of the ACL name
  eace - Hash value or ID of the ACL name
  exace - Hash value or ID of an extended ACE configuration
  username - NSEL username

  NEL (NetFlow Event Logging) fields
  =====================
  nevent - NEL NAT Event
  nsrcport - NEL NAT src port 
  ndstport - NEL NAT dst port
  vrf - NEL NAT ingress vrf id 
  nsrcip - NEL NAT inside address
  ndstip - NEL NAT outside address

  Extra/special fields
  =====================
  cl - nprobe latency client_nw_delay_usec 
  sl - nprobe latency server_nw_delay_usec
  al - nprobe latency appl_latency_usec

=head1 PERFORMANCE

It is obvious tahat prformance of the perl interface is lower comparing to 
highly optimized nfdump utility. As nfdump is able to process up 
to 2 milions of records per second, the Net::NfDump is not bale to process 
more than 1 milion of records per second. However there are several rules 
to keep the code optimised:

=over 

=item * 

Use C<< $obj->fetchrow_arrayref() >> and C<< $obj->storerow_arrayref() >> instead of 
C<< *_array >> and C<< *_hashref >> equivalents. Arrayref handles only the reference 
to the structure with data. Avoid of using C<< *_hashref >> functions, it can by 5 times 
slower.

=item * 

Handle to the perl API only items that are nescessary for using in the code. It is always 
more effective to define in C<< Fields => 'srcip,dstip,...' >> intead of C<< Fileds => '*' >>. 

=item * 

Prefer of using C<< $obj->clonerow($obj2) >> method. This method copies data between 
two instances directly in the C code in the libnf layer. 

Following code: 

  $obj1->exec( Fields => '*' );
  $obj2->create( Fields => '*' );

  while ( my $ref = $obj1->fetchrow_arrayref() ) {
      # do something with srcip 
      $obj2->storerow_arrayref($ref);
  }

can be written in more effective way (several times faster): 

  $obj1->exec( Fields => 'srcip' );
  $obj2->create( Fields => 'srcip' );

  while ( my $ref = $obj1->fetchrow_arrayref() ) {
      # do something with srcip 
      $obj2->clonerow($obj1);
      $obj2->storerow_arrayref($ref);
  }


=back 



=cut 

#=head1 FLOW QUERY - NOT IMPLEMENTED YET
#
#The flow query is language vyry simmilar to SQL to query data on 
#nfdump files. However flow query have nothing to do with SQL. It uses
#only simmilar command syntax. Example of flow query 
#
#  SELECT * FROM data/nfdump1.nfcap, data2/nfdump2.nfcap
#  WHERE src host 147.229.3.10 
#  TIME WINDOW BETWEEN '2012-06-03' AND '202-06-04' 
#  ORDER BY bytes
#  LIMIT 100
#
#
#  INSERT INTO data/nout_nfdump.nfcap (srcip, dstip, srcport, dstport) 
#
#

=pod 

=head1 NOTE ABOUT 32BIT PLATFORMS

Nfdump primary uses 64 bit counters and other items to store single integer value. However 
the native 64 bit support is not compiled in every perl. For thoose cases where 
only 32 integer values are supported the C<Net::NfDump> uses C<Math::Int64> module. 

The build scripts automatically detect the platform and C<Math::Int64> module is required
only on platforms where available perl do not supports 64bit integer values. 

=head1 EXAMPLES OF USE 

There are several examples in the C<examples> directory. 

=over 

=item * 

C<example1.pl> -  The trivial example showing how the C<Net::NfDump> can be used for 
reading files. The exaple also uses the progress bar to show the status 
of processed files. 

=item * 

C<download_asn_db>, C<nf_asn_geo_update> - The set of sripts for updating information 
about AS numbers and country codes based on BGP and geaolocation database. Every flow 
can be extended with src/dst AS number and src/dst country code. 

The firts script (C<download_asn_db>) downloads the BGP database that is available 
on RIPE server. The database then is preprocessed and prepared for second script (with 
support of C<Net::IP::LPM> module).

The sceond script (C<download_asn_db>) updates the AS (or country code) information 
in the nfdump file. It can be run as the extra command (-x option of nfcapd) to update 
information as the new file is available. 

The information about src/dst country works in simmilar way. It uses maxmind database 
and C<Geo::IP> module. However nfdump do not support any field for storing that kinf of 
information the xsrcport andf xdstport fiealds are used indtead. The contry code is 
converted into 16 bit informatiuon (firt 8 bytes for first characted of country code and 
second 8 bytes for second one). 

=back


=head1 SEE ALSO

http://nfdump.sourceforge.net/

=head1 AUTHOR

Tomas Podermanski, E<lt>tpoder@cis.vutbr.czE<gt>, Brno University of Technology

=head1 COPYRIGHT AND LICENSE

Copyright (C) 2012 by Brno University of Technology

This library is free software; you can redistribute it and modify
it under the same terms as Perl itself.

If you are satisfied with using C<Net::NfDump> please send us a postcard, preferably with a picture from your location / city to: 

  Brno University of Technology 
  CVIS
  Tomas Podermanski 
  Antoninska 1
  601 90 
  Czech Republic 

=cut

1;

__END__

