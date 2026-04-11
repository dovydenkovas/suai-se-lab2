#!/bin/sh

binfile=ecampus.cgi
apachefile=ecampus.conf
sitedir=/var/www/ecampus

if [ ! -f "$binfile" ]; then
    echo "Call ecampus after build"
    exit 1
fi

if [ $(id -u) -ne 0 ]; then
    echo "Should be run as root"
    exit 1
fi

install configs/$apachefile /etc/apache2/sites-available/
ln -s /etc/apache2/sites-available/$apachefile /etc/apache2/sites-enabled/
a2enmod cgid
systemctl restart apache2

mkdir -p --mode=755 $sitedir/html
mkdir -p --mode=755 $sitedir/cgi-bin
install -m 644 html/index.html $sitedir/html
install -m 755 $binfile $sitedir/cgi-bin
