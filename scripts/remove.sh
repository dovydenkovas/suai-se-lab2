#!/bin/sh

binfile=ecampus.cgi
apachefile=ecampus.conf
sitedir=/var/www/ecampus

if [ $(id -u) -ne 0 ]; then
    echo "Should be run as root"
    exit 1
fi

rm -rf sitedir
rm /etc/apache2/sites-*/$apachefile
systemctl restart apache2
