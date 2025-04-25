package ZoneMinder::Control::HikVisionExternal;

use strict;
use warnings;
use ZoneMinder::Logger qw(:all);
require ZoneMinder::Base;
require ZoneMinder::Control;

our @ISA     = qw(ZoneMinder::Control);
our $VERSION = '1.00';

# Chemin complet vers votre binaire
my $BINARY = '/opt/ptz_control/ptz_control';

sub open {
    my $self = shift;
    $self->loadMonitor();

    my $addr = $self->{Monitor}{ControlAddress} || '';
    unless ( $addr =~ m{^([^:]+):([^@]+)\@([^:]+):?(\d+)$} ) {
        Error("ControlAddress doit être user:pass\@host:port (actuel='$addr')");
        return;
    }
    ($self->{user}, $self->{pass}, $self->{host}, $self->{port})
      = ($1, $2, $3, $4);
    $self->{port} ||= 8000;

    # On lance le service détaché avec nohup, on capture son PID
    my $log = '/var/log/zm/ptz_service.log';
    my $cmd = join( ' ',
      'nohup',                  # ignore hangups
      $BINARY, 'serve',
      $self->{host}, $self->{port},
      $self->{user}, $self->{pass},
      '>>', $log, '2>&1',       # logs dans ptz_service.log
      '&',                      # background
      'echo $!'                 # renvoie le PID du dernier background
    );

    Info("HikVisionExternal: lancement du service PTZ: $cmd");
    my $pid = qx($cmd);
    chomp $pid;

    unless ( $pid =~ /^\d+$/ ) {
        Error("HikVisionExternal: impossible de lancer ptz_control: $pid");
        return;
    }

    $self->{ptz_pid} = $pid;
    # on laisse un peu le temps à ptz_control de créer son socket
    sleep 1;

    $self->{state}    = 'open';
    $self->{last_cmd} = undef;
    Info("HikVisionExternal: service PTZ lancé (pid=$pid)");
    return 1;
}

sub close {
    my $self = shift;
    if ( my $pid = delete $self->{ptz_pid} ) {
        Info("HikVisionExternal: arrêt du service (pid=$pid)");
        kill 'TERM', $pid;
        # inutile d'attendre, le shell nohup fera le reste
    }
    $self->{state} = 'closed';
    Info("HikVisionExternal fermé");
    return 1;
}

sub sendPTZCommand {
    my ( $self, $cmd, $act ) = @_;
    Info("HikVisionExternal: envoi PTZ $cmd $act");
    my $rc = system( $BINARY, $cmd, $act );
    if ( $rc != 0 ) {
        Error("HikVisionExternal: '$BINARY $cmd $act' failed rc=$rc");
        return 0;
    }
    return 1;
}

sub moveConLeft   { my $s=shift; $s->{last_cmd}='LEFT';  $s->sendPTZCommand('LEFT','START') }
sub moveConRight  { my $s=shift; $s->{last_cmd}='RIGHT'; $s->sendPTZCommand('RIGHT','START') }
sub moveConUp     { my $s=shift; $s->{last_cmd}='UP';    $s->sendPTZCommand('UP','START') }
sub moveConDown   { my $s=shift; $s->{last_cmd}='DOWN';  $s->sendPTZCommand('DOWN','START') }

sub moveStop {
    my $self = shift;
    return unless $self->{state} eq 'open';
    my $cmd = $self->{last_cmd}
      or return Error("moveStop sans commande précédente");
    $self->sendPTZCommand( $cmd, 'STOP' );
}

1;
