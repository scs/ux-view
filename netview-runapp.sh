#! /bin/msh

# Register the application with inetd.
echo "netview 9999/tcp" >> /etc/services
echo "netview stream tcp wait root /mnt/app/netview_target" >> /etc/inetd.conf

killall inetd
inetd &
