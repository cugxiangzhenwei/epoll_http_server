#!/bin/bash
mysql -u root -p -D netdisk -e "show tables;desc t_user;desc t_file; desc t_filelist; desc t_block;"
