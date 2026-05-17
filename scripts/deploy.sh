#!/bin/bash

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
source .env
sed -i /etc/apache2/sites-available/$apachefile -e "s|DB_CONNECTION_SECRET|'$DB_CONNECTION_SECRET'|"

ln -sf /etc/apache2/sites-available/$apachefile /etc/apache2/sites-enabled/
/usr/sbin/a2enmod cgid 2> /dev/null
/usr/sbin/a2enmod headers 2> /dev/null
/usr/sbin/a2enmod rewrite 2> /dev/null
systemctl restart apache2

chown www-data /srv
mkdir -p --mode=755 $sitedir/html
mkdir -p --mode=755 $sitedir/cgi-bin
install -m 644 html/index.html $sitedir/html
install -m 755 $binfile $sitedir/cgi-bin
