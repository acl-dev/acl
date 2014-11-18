CREATE DATABASE `db_ctl`;
USE `db_ctl`;

/* 该表定义了 数据库|数据表|索引 的字符串名称与整数值的映射表 */
CREATE TABLE `tbl_name_type` (
	`id` int(10) unsigned NOT NULL AUTO_INCREMENT COMMENT '对应 name 的ID号',
	`name` varchar(64) NOT NULL COMMENT '数据库|数据表|索引的名称之一',
	`type` tinyint(1) unsigned NOT NULL COMMENT '该条记录的名称类型: 0-数据库，1-表，2-索引',
	PRIMARY KEY(`id`) USING BTREE,
	UNIQUE KEY `Index_name_type` (`name`, `type`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

/* 索引服务器信息表，记录着主机ID号，地址，存储记录数 */
CREATE TABLE `tbl_idx_host` (
	`id` int(10) unsigned NOT NULL AUTO_INCREMENT COMMENT '索引服务器唯一标识号',
	`addr` varchar(32) NOT NULL COMMENT '主机地址，IP:PORT 或域套接口',
	`count` bigint(20) NOT NULL COMMENT '已经存储的总记录数',
	PRIMARY KEY(`id`) USING BTREE,
	UNIQUE KEY `Index_addr` (`addr`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

/* 数据服务器信息表，记录着主机ID号，地址，存储记录数 */
CREATE TABLE `tbl_dat_host` (
	`id` int(10) unsigned NOT NULL AUTO_INCREMENT COMMENT '数据服务器唯一标识号',
	`addr` varchar(32) NOT NULL COMMENT '主机地址，IP:PORT 或域套接口',
	`priority` int(10) NOT NULL COMMENT '服务器负载权重，值越高表示负载越低',
	`count` bigint(20) NOT NULL COMMENT '已经存储的总记录数',
	PRIMARY KEY(`id`) USING BTREE,
	UNIQUE KEY `Index_addr` (`addr`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

/* 从属于数据库中的索引数据库主机列表 */
CREATE TABLE `tbl_db_host` (
	`id_db` int(10) unsigned NOT NULL COMMENT '数据库ID号，对应 tbl_name 中 type 类型值为 0 的 id',
	`id_idx_host` int(10) unsigned NOT NULL COMMENT '索引服务器ID号，对应 tbl_idx_host 中的 id',
	`count` bigint(20) unsigned NOT NULL COMMENT '该数据库在主机中的记录总数',
	PRIMARY KEY(`id_db`, `id_idx_host`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

/* 从属于数据库中的数据表列表 */
CREATE TABLE `tbl_db_tbl` (
	`id_db` int(10) unsigned NOT NULL COMMENT '数据库ID号，对应 tbl_name 中 type 类型值为 0 的 id',
	`id_tbl` int(10) unsigned NOT NULL COMMENT '数据表名ID号，对应 tbl_name 中 type 类型值为 1 的 id',
	`count` bigint(20) NOT NULL COMMENT '该数据表中记录总数',
	PRIMARY KEY(`db_name`, `tbl_name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

/* 数据表中索引集合表 */
CREATE TABLE `tbl_tbl_idx` (
	`id_idx` int(10) unsigned NOT NULL COMMENT '数据表中索引ID号，对应 tbl_name 中 type 类型值为 2 的 id',
	`id_db` int(10) unsigned NOT NULL COMMNET '数据库ID号，对应 tbl_name 中 type 类型值为 0 的 id',
	`id_tbl` int(10) unsigned NOT NULL COMMENT '数据表ID号，对应 tbl_name 中 type 类型值为 1 的 id',
	`unique` tinyint(1) unsigned NOT NULL COMMENT '该索引在数据表中是否是唯一索引类型',
	`type` tinyint(1) unsigned NOT NULL COMMENT '索引类型：0 - 字符串，1 - 布尔型，2 - 16位整数，3 - 32位整数，4 - 64位整数',
	PRIMARY KEY(`id_idx`, `id_db`, `id_tbl`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
