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

our $VERSION = '1.03';
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

  my $lpm = Net::IP::LPM->new();

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


  printf $lpm->lookup('147.229.100.100'); # returns net1
  printf $lpm->lookup('147.229.3.10');    # returns host3
  printf $lpm->lookup('2001:67c:1220::1');# returns net16
  

=head1 DESCRIPTION

The module Net::IP::LPM implements the Longest Prefix Matxh algo 
for both IPv4 and IPv6 protocols.  The module uses Trie algo. 

=head1 PERFORMANCE

The module is able to match  ~ 1 mln. lookups  
per second on complete Internet BGP table (aprox 500 000 prefixes) on ordinary 
hardware (2.4GHz Xeon CPU). For more detail you can try make test on module source
to check performance on your system. Module supports both IPv4 and IPv6 protocol.

=head1 CLASS METHODS


=head2  new - Class Constructor


  $lpm = Net::IP::LPM->new( );

Constructs a new Net::IP::LPM object. 

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
	my ($addr) = @_;

	if ((my $addr_bin = inet_pton(AF_INET, $addr))) {
		return $addr_bin;
	} else {
		return inet_pton(AF_INET6, $addr);
	}

}

=head1 OBJECT METHODS

=head2 add - Add Prefix

   $code = $lpm->add( $prefix, $value );

Adds prefix B<$prefix> into database with value B<$value>. Returns 1 if 
the prefix was added sucesfully. Returns 0 when some error happens (typically wrong address formating).

=cut 
sub add {
	my ($self, $prefix, $value) = @_;

#	printf "PPP: %s %s %s\n", $self->{handle}, $prefix, $value;
	my ($prefix_bin, $prefix_len);

	($prefix, $prefix_len) = split('/', $prefix);	

	if (! ($prefix_bin = inet_pton(AF_INET, $prefix)) ) {
		$prefix_bin = inet_pton(AF_INET6, $prefix);
	}

	if (!defined($prefix_len)) {
		if (length($prefix_bin) == 4) {
			$prefix_len = 32;
		} else {
			$prefix_len = 128;
		}
	}

	return lpm_add_raw($self->{handle}, $prefix_bin, $prefix_len, $value);
}

# legacy code
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
	my $addr_bin;

	if (! ($addr_bin = inet_pton(AF_INET, $addr)) ) {
		$addr_bin = inet_pton(AF_INET6, $addr);
	}

	return lpm_lookup_raw($self->{handle}, $addr_bin);
#	return lpm_lookup($self->{handle}, $addr);
}

=head2  lookup_raw - Lookup Address in raw format

 
  $value = $lpm->lookup_raw( $address );

Same as C<$lpm-E<gt>lookup> but takes $address in raw format (result of inet_ntop function). It is 
more effective than C<$lpm-E<gt>lookup>, because convertion from text format is not 
nescessary. 

=cut 

sub lookup_raw {
#	my ($self, $addr_bin) = @_;

	return lpm_lookup_raw($_[0]->{handle}, $_[1]);
}

# legacy code

sub lookup_cache_raw {
#	my ($self, $addr_bin) = @_;

	return lpm_lookup_raw($_[0]->{handle}, $_[1]);

}

=head2  info - Returns information about builded trie 

  $ref = $lpm->info();

Returns following items 

  ipv4_nodes_total - total number of allocated nodes in trie
  ipv4_nodes_value - number of allocated nodes in trie that have stored some value 
  ipv4_trie_bytes - number of allocated for trie nodes (withnout data)
  ipv6_ - same for IPv6 

=cut 

sub info {
	my ($self) = @_;

	return lpm_info($self->{handle});
}	

=head2  finish - Release all data in object

 
  $lpm->finish();

=cut 
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
