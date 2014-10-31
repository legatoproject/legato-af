#!/usr/bin/perl -w

use XML::LibXML;
use Data::Dumper;

use Socket;
use IO::Handle;

my %config;
my $RequestCpt=0;

sub createConfig
{
    my $file=$_[0];
    
    my $parser = XML::LibXML->new();
    my $tree = $parser->parse_file($file);
    
    print Dumper $tree;

    for my $Tests ( $tree->findnodes('/tests') )
    {
        print $Tests->nodeName(),"\n";
        
        for my $childtest ($Tests->getChildnodes) {
            print $childtest->nodeName()," is ",$childtest->nodeType(),"\n";
            if ( $childtest->nodeType() == XML_ELEMENT_NODE ) {
                print "\t",$childtest->nodeName()," : ",$childtest->textContent(),"\n";
                
                my $command = $childtest->findvalue('./ask');                
                print "\t\t cmd: $command \n";
                
                my @resp = $childtest->findnodes($childtest->nodePath."/resp/ack");
                for my $ack (@resp) {
                    print "\t\t\t ack : ",$ack->textContent," \n";
                }
                
                push(@{${config{$command}}},\@resp);
                $RequestCpt++
            } else {
                print "NOT USED\n";
            }
        }

#         my $idx = $doc->{$Test}->{ask};
#         my $val = $doc->{$Test}->{resp}->{ack};
#         print "test $Test\n";
#         print "\t ask -> $idx\n";
#         print "\t ack -> $val\n";
#         
#         push(@{${config{$idx}}},$doc->{$Test}->{resp}->{ack});
#         $RequestCpt++;
    }
}

sub printConfig
{
    foreach $cle(sort keys %config)
    {
        print "key: $cle\n";
#         print Dumper @{${config{$cle}}};
        foreach $idx (keys @{${config{$cle}}}) {
            print "idx: $idx\n";
            foreach $elem (@{${config{$cle}[$idx]}}) {
                print "\t ",$elem->textContent,"\n";
            }
        }
    }
    print "Number of response $RequestCpt \n";
}

sub launchDaemon
{
    my $file=$_[0];
    
    socket(SERV, PF_UNIX, SOCK_STREAM,0);
    if (-e $file)
    {
        print "$file exists, remove it\n";
        unlink $file;
    }
    bind(SERV,sockaddr_un($file)) or print "ERROR!";

    listen(SERV,1);
    while (accept(CLIENT,SERV))
    {
        local $/ = "\r";
        CLIENT->autoflush(1);
        while (<CLIENT>) {
            my $line = $_;
            
            chomp($line);
            print "Received >$line<\n";
            foreach $cle(sort keys %config)
            {
                chomp($cle);
                print "$line ##### $cle\n";
                if ("$line" eq "$cle")
                {
                    $elem = shift(${config{$cle}});
                    print "ref ",ref($elem),"\n";
                    print "found -$cle- -$elem->textContent-\n";
                    if (ref($elem) eq ARRAY ) {
                       foreach $l ( @{$elem} ) {
                            if ($l->textContent eq ">")
                            {
                                my $old_input_rec_sep = $/;
                                print "\t -",$l->textContent,"- to send without return\n";
                                print CLIENT "\r\n",$l->textContent;

                                $/ = "\x1A";
                                $_ = <CLIENT>;
                                
                                print "\t -",$_,"- received\n";
#                                 print CLIENT "\r\n",$l->textContent;

                                $/ = $old_input_rec_sep;
                            }
                            else
                            {
                                print "\t -",$l->textContent,"- to send\n";
                                print CLIENT "\r\n",$l->textContent,"\r\n";
                            }
                        }
                        $RequestCpt--;
                    }
                    else {
                        print "Error no response for this line -$cle-\n"
                    }
                }
            }
            print "$RequestCpt End of $line<\n";

#                 print Dumper %config;
            
            if ($RequestCpt == 0) {
                sleep(1);
                close CLIENT;
                close SERV;
                unlink $file;
                exit;
            }
        }
    }
}

$num_args = $#ARGV + 1;
if ($num_args != 2) {
  print "Usage: modem.pl configFile pipe\n";
  exit;
}

createConfig($ARGV[0]);
printConfig();
launchDaemon($ARGV[1]);
