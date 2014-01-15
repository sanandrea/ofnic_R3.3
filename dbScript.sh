#!/bin/bash
echo -n Please enter the privileged mysql username : 
read username
echo -n Please enter the mysql password of user $username : 
read -s password
echo ...
mysql -u$username -p$password -e "DROP DATABASE openflow_users"
mysql -u$username -p$password -e "CREATE DATABASE openflow_users"
mysql -u$username -p$password -e "GRANT ALL PRIVILEGES ON openflow_users.* TO ofnic IDENTIFIED BY 'openflow'"
mysql --user=$username --password=$password openflow_users < db_dump.sql
