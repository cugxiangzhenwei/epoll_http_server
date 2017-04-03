drop database if exists netdisk;

create database netdisk;

use netdisk;

/*用户信息表*/
create table t_user
(
 user_xid int unsigned primary key not null auto_increment,
 user_name varchar(50) not null,
 user_nickname varchar(100) not null,
 user_pwd varchar(15) not null,
 user_sex char(1) not null,
 user_phone char(11) not null,
 user_mail char(40) not null,
 user_filetable varchar(100) not null,
 user_space int unsigned not null,
 user_root_pxid int unsigned not null,
 create_date datetime not null
 );

/*文件列表集合*/
create table t_filelist
(
 file_xid int unsigned primary key not null auto_increment,
 file_pxid int unsigned not null,
 file_name varchar(255) not null,
 file_ctime int unsigned not null,
 file_mtime int unsigned not null,
 file_md5 varchar(128) not null,
 file_size int unsigned not null,
 file_ver int unsigned not null,
 user_xid int unsigned  not null,
 foreign key(user_xid) references t_user(user_xid) on update cascade
);

/*文件数据块描述记录表*/
create table t_file
(
 file_md5 varchar(128) primary key not null,
 file_size int unsigned not null,
 file_blocks longtext
);

/*数据块表,代表文件中的一个数据块*/
create table t_block
(
block_md5 varchar(128) primary key not null,
block_size int unsigned not null,
block_xid int unsigned not null,
block_data MEDIUMBLOB not null
);

