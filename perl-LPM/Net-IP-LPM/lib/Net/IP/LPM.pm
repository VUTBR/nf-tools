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

our $VERSION = '1.01';
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

#XXX    if ($] >= 5.00561) {
#XXX        *$AUTOLOAD = sub () { $val };
#XXX    }
#XXX    else {
		*$AUTOLOAD = sub { $val };
#XXX    }
	}
	goto &$AUTOLOAD;
}

require XSLoader;
XSLoader::load('Net::IP::LPM', $VERSION);

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
	
	$self->{handle} = lpm_init();

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

#	printf "PPP: %s %s %s\n", $self->{handle}, $prefix, $value;
	return lpm_add($self->{handle}, $prefix, $value);
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
}

=head2  lookup - Lookup Address

 
  $value = $lpm->$lookup( $address );

Lookups the prefix in the database and returns the value. If the prefix is
not found or error ocured the undef value is returned. 

Before lookups are performed the database have to be rebuilded by C<$lpm-E<gt>rebuild()> operation. 

=cut 

sub lookup {
	my ($self, $addr) = @_;

	return lpm_lookup($self->{handle}, $addr);
}

=head2  lookup_raw - Lookup Address in raw format

 
  $value = $lpm->lookup_raw( $address );

Same as C<$lpm-E<gt>lookup> but takes $address in raw format (result of inet_ntop function). It is 
more effective than C<$lpm-E<gt>lookup>, because convertion from text format is not 
nescessary. 

=cut 

sub lookup_raw {
	my ($self, $addr_bin) = @_;

	return lpm_lookup_raw($self->{handle}, $addr_bin);
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

	return lpm_lookup_raw($self->{handle}, $addr_bin);

#	my $result = $self->{CACHE}->{$addr_bin};

#	if (!defined($result)) {
#		$result = $self->_lookup_raw($addr_bin);
#		$self->{CACHE}->{$addr_bin} = $result;
#	}

#	return undef if ( ! defined($result) || length($result) == 0 );
#	return $result;
}

sub finish {
	my ($self) = @_;

	lpm_finish($self->{handle});
}	

sub DESTROY {
	my ($self) = @_;

	lpm_destroy($self->{handle});
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
