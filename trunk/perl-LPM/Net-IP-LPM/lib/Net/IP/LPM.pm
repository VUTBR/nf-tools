package Net::IP::LPM;

#use 5.010001;
use strict;
use warnings;
use Carp;

require Exporter;
#use AutoLoader;

use Socket qw( AF_INET );
use Socket6 qw( inet_ntop inet_pton AF_INET6 );
use Data::Dumper;
use DB_File;
use constant {
	V4P => pack('C', AF_INET),
	V6P => pack('C', AF_INET6),
	VF => pack('C', 0xF),
	V0 => pack('C', 0x0),
	};
#our @ISA = qw(DB_File);
our @ISA = qw();

# Items to export into callers namespace by default. Note: do not export
# names by default without a very good reason. Use EXPORT_OK instead.
# Do not simply export all your public functions/methods/constants.

# This allows declaration	use Net::IP::LPM ':all';
# If you do not need this, moving things directly into @EXPORT or @EXPORT_OK
# will save memory.
our %EXPORT_TAGS = ( 'all' => [ qw(
	
) ] );

our @EXPORT_OK = ( @{ $EXPORT_TAGS{'all'} } );

our @EXPORT = qw(
	
);

our $VERSION = '0.01_03';


# Preloaded methods go here.

# Autoload methods go after =cut, and are processed by the autosplit program.

# Below is stub documentation for your module. You'd better edit it!

=head1 NAME

Net::IP::LPM - Perl implementation of Longest Prefix Match algorithm

=head1 SYNOPSIS

  use Net::IP::LPM;

  my $lpm = Net::IP::LPM->new("prefixes.db");

  # add prefixes 
  $lpm->add('0.0.0.0/0', 'default');
  $lpm->add('::/0', 'defaultv6');
  $lpm->add('147.229.0.0/16', 'net1');
  $lpm->add('147.229.3.0/24', 'net2');
  $lpm->add('147.229.3.10/32', 'host3');
  $lpm->add('147.229.3.11', 'host4');
  $lpm->add('2001:67c:1220::/32', 'net16');
  $lpm->add('2001:67c:1220:f565::/64', 'net26');
  $lpm->add('2001:67c:1220:f565::1235/128', 'host36');
  $lpm->add('2001:67c:1220:f565::1236', 'host46');

  # rebuild the database
  $lpm->rebuild();

  printf $lpm->lookup('147.229.100.100'); # returns net1
  printf $lpm->lookup('147.229.3.10');    # returns host3
  printf $lpm->lookup('2001:67c:1220::1');# returns net16
  

=head1 DESCRIPTION

The module Net::IP::LPM implements the Longest Prefix Matxh algo 
for both IPv4 and IPv6 protocols. Module divides prefixes into 
intervals of numbers and them uses range search to match proper 
prefix. 


Module also allows to store builded database into file. It is usefull 
when the module is used on large database (for example full BGP tables 
containing over half of milion records) when initial building can take 
long time (a few seconds). The separate script can  
download and prepare prebuilded database and another one can just use
the prepared database and perform fast lookups. 

=head1 PERFORMANCE

The module is able to match  ~ 180 000 lookups (or 300 000 with cache enables) 
per second on complete Internet BGP table (aprox 500 000 prefixes) on ordinary 
hardware (2.4GHz Xeon CPU). For more detail you can try make test on module source
to check performance on your system.

=head1 CLASS METHODS


=head2  new - Class Constructor


  $lpm = Net::IP::LPM->new( [ $filename ] );

Constructs a new Net::IP::LPM object. The F<$filename> can be specified as the 
database of prebuilded prefixes. If the file name is not handled the whole 
structure will be stored into the memory. 

=cut 
sub new {
	my ($class, $dbfile) = @_;
	my %h;
	my $self = {};
	
	#my $class = $self->SUPER::new( @opts );
	my $b = new DB_File::BTREEINFO ;
#	$b->{'compare'} = sub { return $_[1] cmp $_[0]; };
#	$b->{'cachesize'} = 40000000;
#	$b->{'cachesize'} = 40000000;

	$self->{DB} = tie %h, 'DB_File', $dbfile, O_RDWR|O_CREAT, 0666, $b ;

	bless $self, $class;
	return $self;
}

# converts IPv4 and IPv6 address into common format 
sub format_addr {
	my ($self, $addr) = @_;

	my ($type);
		
	if (index($addr, ':') != -1) {
		$type = AF_INET6;	
	} else {
		$type = AF_INET;
	}

	my $addr_bin = inet_pton($type, $addr);

	return ($type, $addr_bin);
	
}

=head1 OBJECT METHODS

=head2 add - Add Prefix


   $code = $lpm->add( $prefix, $value );

Adds prefix B<$prefix> into database with value B<$value>. Returns 1 if 
the prefix was added sucesfully. Returns 0 when some error happens (typically wrong address formating).

After adding prefixes rebuild of database have to be performed. 
=cut 
sub add {
	my ($self, $prefix, $value) = @_;

	# IPv4 addresses (x.x.x.x/p) are converted to IPv6 ::x.x.x.x/(128 - 32 + p) format 
	my ($addr, $plen) = split('/', $prefix); 

	my ($type, $addr_bin) = format_addr($self, $addr);
	$addr_bin = pack('C', $type).$addr_bin;

	if (!defined($plen)) {
		$plen = 128 if ($type == AF_INET6);
		$plen = 32 if ($type == AF_INET);
	}

#	if ($type == AF_INET) {
#		$plen = 128 - 32 + $plen;
#	}

	# invalid conversion
	return undef if ( !defined($addr_bin) || !defined($value) );

	# add to prefix structure 
	$self->{PREFIXES}->{$plen}->{$addr_bin} = $value ; 

	return 1;
}

sub get_range {
	my ($self, $prefix, $plen) = @_;

	my ($type, $addr_bin) = unpack("Cb*", $prefix);

	my $addrlen = ($type == AF_INET6) ? 128 : 32;
	
	my $first = substr($addr_bin, 0, $plen) . "0"x($addrlen - $plen);
	my $last = substr($addr_bin, 0, $plen) . "1"x($addrlen - $plen);

	return (pack("Cb*", $type, $first).V0, pack("Cb*", $type, $last).VF);
}
	
=head2 rebuild - Rebuild Prefix Database

 
  $code = $lpm->rebuild();

Rebuilds the database. After adding new prefixes the database have to 
be rebuilded before lookups are performed. Depends on the F<$filename>
handled in the constructor the database will be stored into F<$filename> or
keept in the memory if the file name was nos specified. 

Returns 1 if the the rebuild was succesfull or 0 if something wrong happend. 

=cut 

sub rebuild {
	my ($self, $addr) = @_;

	# initalize whole range as undef
	$self->{DB}->put(pack('C', AF_INET6).inet_pton(AF_INET6, '::').V0, undef);
	$self->{DB}->put(pack('C', AF_INET6).inet_pton(AF_INET6, 'FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF').VF, undef);

	$self->{DB}->put(pack('C', AF_INET).inet_pton(AF_INET, '0.0.0.0').V0, undef);
	$self->{DB}->put(pack('C', AF_INET).inet_pton(AF_INET, '255.255.255.255').VF, undef);

	foreach my $plen ( sort { $a <=> $b } keys %{$self->{PREFIXES}} ) {
		while ( my ($prefix, $value) = each ( %{$self->{PREFIXES}->{$plen}} ) ) {
			my ($first, $last) = $self->get_range($prefix, $plen);
			
			# try to find longer prefix
			my $up_val = undef;
			my $up_key = $first.V0;
    		my $st1 = $self->{DB}->seq($up_key, $up_val, R_CURSOR);
    		my $st2 = $self->{DB}->seq($up_key, $up_val, R_PREV);

#			printf "NEW: %s , %s -> %s\n", unpack("H*", $first), unpack("H*", $last), $value;
#			printf "UP : %s -> %s\n", unpack("H*", $up_key), $up_val;

			#  inserted prefix            first |-----value-----| last 
			#  up prefix        up_key |----------up_val------------------| 
			#  result           up_key |-up_val-|-----value-----|-up_val----|
			#                                   first           last + 1 	
			if ($st1 == 0 && $st2 == 0) {
#			if ($st1 == 0) {
				$self->{DB}->put($last, $up_val);
				$self->{DB}->put($first, $value);
			} else {	# prefix not in a database yet 
				croak "This should never happen!!";
			}
		}
	}
}

=head2  lookup - Lookup Address

 
  $value = $lpm->$lookup( $address );

Lookups the prefix in the database and returns the value. If the prefix is
not found or error ocured the undef value is returned. 

Before lookups are performed the database have to be rebuilded by C<$lpm-E<gt>rebuild()> operation. 

=cut 

sub lookup {
	my ($self, $addr) = @_;

	my ($type, $addr_bin) = format_addr($self, $addr);

	return undef if (! defined($addr_bin) );

	return $self->lookup_raw($addr_bin);
}

=head2  lookup_raw - Lookup Address in raw format

 
  $value = $lpm->lookup_raw( $address );

Same as C<$lpm-E<gt>lookup> but takes $address in raw format (result of inet_ntop function). It is 
more effective than C<$lpm-E<gt>lookup>, because convertion from text format is not 
nescessary. 

=cut 

sub lookup_raw {
	my ($self, $addr_bin) = @_;

	if (length($addr_bin) == 4) {
		$addr_bin = V4P.$addr_bin.VF;
	} else {
		$addr_bin = V6P.$addr_bin.VF;
	}

	my ($key, $value);
	
    my $st1 = $self->{DB}->seq($addr_bin, $value, R_CURSOR);
    my $st2 = $self->{DB}->seq($addr_bin, $value, R_PREV);

	if ($st1 == 0 && $st2 == 0 ) {
		return $value;
	}

	return undef;
}

=head2  lookup_cache_raw - Lookup Address in raw format with cache

 
  $value = $lpm->lookup_cache_raw( $address );

Same as C<$lpm-E<gt>lookup_raw> but the cache is used to speed up lookups. 
It might be usefull when there is big probability that lookup 
for the same address will be porformed more often.  

The cache becomes effective when the ratio of hit cache entries is bigger 
than 50%. For less hit ratio the overhead discard the cache benefits. 

NOTE: Cache entries are stored into memory, so it can lead to unexpected memory comsumption. 

=cut 

sub lookup_cache_raw {
	my ($self, $addr_bin) = @_;

	my $result = $self->{CACHE}->{$addr_bin};

	if (!defined($result)) {
		$result = $self->lookup_raw($addr_bin);
		$self->{CACHE}->{$addr_bin} = $result;
	}

	return $result;
}	

=head1 SEE ALSO

There are also other implementation of Longest Prefix Matxh in perl. However 
most of them have some disadvantage (poor performance, lack of support for IPv6
or requires a lot of time for initial database building). However in some cases 
it might be usefull:

L<Net::IPTrie>

L<Net::IP::Match>

L<Net::IP::Match::Trie>

L<Net::IP::Match-XS>

L<Net::CIDR::Lookup>

L<Net::CIDR::Compare>

=head1 AUTHOR

Tomas Podermanski E<lt>tpoder@cis.vutbr.czE<gt>
Brno University of Technology

=head1 COPYRIGHT AND LICENSE

Copyright (C) 2012, Brno University of Technology

This library is free software; you can redistribute it and/or modify
it under the same terms as Perl itself.


=cut

1;
__END__
