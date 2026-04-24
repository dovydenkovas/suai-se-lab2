#!/bin/sh

binfile=api.cgi
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
ln -sf /etc/apache2/sites-available/$apachefile /etc/apache2/sites-enabled/
/usr/sbin/a2enmod cgid
/usr/sbin/a2enmod headers
/usr/sbin/a2enmod rewrite
systemctl restart apache2

chown www-data /srv
mkdir -p --mode=755 $sitedir/html
mkdir -p --mode=755 $sitedir/cgi-bin
install -m 644 html/index.html $sitedir/html
install -m 755 $binfile $sitedir/cgi-bin
