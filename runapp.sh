#! /bin/msh

# Register the application with inetd.
echo "netview 9999/tcp" >> /etc/services
echo "netview stream tcp wait root /app/netview" >> /etc/inetd.conf

killall inetd
inetd &
