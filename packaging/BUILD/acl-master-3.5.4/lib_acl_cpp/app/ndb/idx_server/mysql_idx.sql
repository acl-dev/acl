CREATE DATABASE `db_name`;
USE `db_name`;

CREATE TABLE `tbl_dat` (
	`id_database` int(10) unsigned NOT NULL COMMENT '数据库名称ID',
	`id_table` int(10) unsigned NOT NULL COMMENT '数据表名称ID',
	`id_index` int(10) unsigned NOT NULL '数据索引名称ID',
	`key` varchar(220) NOT NULL '索引字段值',
	`id_dat`, bigint(20) unsigned NOT NULL COMMENT '存储唯一ID号',
	PRIMARY KEY(`id_database`, `id_table`, `id_index`, `key`) USING BTREE,
	KEY `Index_id_dat` (`id_dat`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

或

CREATE TABLE `tbl_dat` (
	`key` varchar(256) NOT NULL COMMENT '由 id_database-id_table-id_index-id:key 拼接而成',
	`id_dat`, bigint(20) unsigned NOT NULL COMMENT '存储唯一ID号',
	PRIMARY KEY(`key`) USING BTREE,
	KEY `Index_id_dat` (`id_dat`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
