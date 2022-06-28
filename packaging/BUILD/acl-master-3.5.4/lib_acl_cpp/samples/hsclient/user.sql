use test;
create table user
(
	user_id integer not null,
	user_name varchar(256) not null,
	user_email varchar(256) not null,
	primary key(user_id),
	index(user_name)
)
