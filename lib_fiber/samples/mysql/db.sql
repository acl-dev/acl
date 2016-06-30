
create table group_tbl
(	group_name varchar(128) not null
	uvip_tbl varchar(32) not null default 'uvip_tbl',
	access_tbl varchar(32) not null default 'access_tbl',
	access_week_tbl varchar(32) not null default 'access_week_tbl',
	access_month_tbl varchar(32) not null default 'access_month_tbl',
	update_date date not null default '1970-1-1',
	disable integer not null default 0,
	add_by_hand integer not null default 0,
	class_level integer not null default 0,
	primary key(group_name, class_level)
);
