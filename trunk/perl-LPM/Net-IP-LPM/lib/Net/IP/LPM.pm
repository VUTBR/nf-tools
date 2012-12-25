package Net::IP::LPM;

use 5.010001;
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

our $VERSION = '0.01';


# Preloaded methods go here.

# Autoload methods go after =cut, and are processed by the autosplit program.

# Below is stub documentation for your module. You'd better edit it!

=head1 NAME

Net::IP::LPM - Perl implementation of Longest Prefix Match 

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
for bosth IPv4 and IPv6 protocols. Module divides prefixes into 
intervals of numbers and them uses range search to match proper 
prefix. 


Module also allows to store builded database into file. It is usefull 
when the module is used on large database (for example full BGP tables 
containing over half of milion records). The separate script can  
download and prepare prebuilded database and another one can just use
the prepared database and perform fast lookups. 

=head1 PERFORMANCE

The module is able to match  prox 30 000 lookups per second on 
complete Internet BGP table (aprox 500 000 prefixes) on ordinary 
hardware (2.4GhZ Xeon CPU)

=head1 CLASS METHODS


=head2  new - Class Constructor

   $lpm = Net::IP::LPM->new( [ $filename ] );

Constructs a new Net::IP::LPM object. The $filename can be specified as the 
database of prebuilded prefixes. If the file name is not handled the whole 
structure will be stored into memory 

=cut 
sub new {
	my ($class, $dbfile) = @_;
	my %h;
	my $self = {};
	
	#my $class = $self->SUPER::new( @opts );
	my $b = new DB_File::BTREEINFO ;
	$b->{'compare'} = sub { return $_[1] cmp $_[0]; };

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

Adds prefix $prefix into database with value $value. Returns 1 if 
the prefix was added sucesfully. Returns 0 if some error happen (typically wrong address formating).

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

	return (pack("Cb*", $type, $first), pack("Cb*", $type, $last));
}
	
=head2 rebuild - Rebuild Prefix Database
 
   $code = $lpm->rebuild();

Rebuilds the database. After adding new prefixes the database have to 
be rebuilded before lookups are performed. Depend on the $filename 
handled in the constructor the database is stored into $filename of 
keept in the memory if the file name was nos specified. 

Returns 1 if the the rebuild was succesfull or 0 if something wrong happend. 

=cut 

sub rebuild {
	my ($self, $addr) = @_;

	# initalize whole range as undef
	$self->{DB}->put(pack('C', AF_INET6).inet_pton(AF_INET6, '::')."0", undef);
	$self->{DB}->put(pack('C', AF_INET6).inet_pton(AF_INET6, 'FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF')."1", undef);

	$self->{DB}->put(pack('C', AF_INET).inet_pton(AF_INET, '0.0.0.0')."0", undef);
	$self->{DB}->put(pack('C', AF_INET).inet_pton(AF_INET, '255.255.255.255')."1", undef);

	foreach my $plen ( sort { $a <=> $b } keys %{$self->{PREFIXES}} ) {
		while ( my ($prefix, $value) = each ( %{$self->{PREFIXES}->{$plen}} ) ) {
			my ($first, $last) = $self->get_range($prefix, $plen);
			
			# try to find longer prefix
			my $up_val = undef;
			my $up_key = $first."0";
    		my $st = $self->{DB}->seq($up_key, $up_val, R_CURSOR);

#			printf "NEW: %s , %s -> %s\n", unpack("H*", $first), unpack("H*", $last), $value;
#			printf "UP : %s -> %s\n", unpack("H*", $up_key), $up_val;

			#  inserted prefix            first |-----value-----| last 
			#  up prefix        up_key |----------up_val------------------| 
			#  result           up_key |-up_val-|-----value-----|-up_val----|
			#                                   first           last + 1 	
			if ($st == 0) {
				$self->{DB}->put($last."1", $up_val);
				$self->{DB}->put($first."0", $value);
			} else {	# prefix not in a database yet 
				croak "This should never happen!!";
			}
		}
	}
}

=head2  lookup - Lookup Address
 
   $value = $lpm->$lookup( $address );

Lookups the prefix in the database and returns the value. If the prefix is
not found or error ocured undef value is returned. 

Before lookups are performed the database have to be rebuilded by $lpm->rebuild() operation. 

=cut 

sub lookup {
	my ($self, $addr) = @_;

	my ($type, $addr_bin) = format_addr($self, $addr);

	return $self->lookup_raw($addr_bin);
}

=head2  lookup_raw - Lookup Address
 
   $value = $lpm->lookup_raw( $address );

Same as $lpm->lookup but takes $address in raw format (result of inet_ntop function). It is 
more effective than $lpm->lookup, because convertion from text format is not 
nescessary. 

=cut 

sub lookup_raw {
	my ($self, $addr_bin) = @_;

	if (length($addr_bin) == 4) {
		$addr_bin = V4P.$addr_bin;
	} else {
		$addr_bin = V6P.$addr_bin;
	}

	return undef if (! defined($addr_bin) );

	my $value = undef;
    my $st = $self->{DB}->seq($addr_bin.'0', $value, R_CURSOR) ;
	
	if ($st == 0) {
		return $value;
	}

	return undef;
}


=head1 SEE ALSO

There are also other implementation of Longest Prefix Matxh in perl. However 
most of them have some disadvantage (poor performance, lack of support for IPv6
or requires a lot of time for initial database building). However for some 
projects might be usefull:

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
it under the same terms as Perl itself, either Perl version 5.10.1 or,
at your option, any later version of Perl 5 you may have available.


=cut

1;
__END__
