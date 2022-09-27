========================================================================
    控制台应用程序 : mysql 项目概况
========================================================================

应用程序向导已为您创建了这个 mysql 应用程序。
此文件包含组成 mysql 应用程序
的每个文件的内容摘要。


mysql.vcproj
    这是用应用程序向导生成的 VC++ 项目的主项目文件。
    它包含有关生成此文件的 Visual C++ 版本的信息，以及
    有关用应用程序向导选择的
    平台、配置和项目功能的信息。

mysql.cpp
    这是主应用程序源文件。

/////////////////////////////////////////////////////////////////////////////
其他标准文件:

StdAfx.h、StdAfx.cpp
    这些文件用于生成名为 mysql.pch
    的预编译头(PCH)文件以及名为 StdAfx.obj 的预编译类型文件。

/////////////////////////////////////////////////////////////////////////////
其他注释:

应用程序向导使用 "TODO:" 注释指示应添加或自定义的源代码部分。

/////////////////////////////////////////////////////////////////////////////

# For old Mysql
use mysql;
create database acl_test_db;
insert into user (User, Host, Password) values ('acl_user', 'localhost', PASSWORD('111111'));
insert into user (User, Host, Password) values ('acl_user', '192.168.1.%', PASSWORD('111111'));

grant CREATE,DROP,INSERT,DELETE,UPDATE,SELECT on acl_test_db.* to 'acl_user';
flush privileges;

# For new Mysql(version > 5.1)
use mysql
UPDATE User SET authentication_string=PASSWORD('1qazxsw2') WHERE user='root';
flush privileges;
exit;

use mysql
ALTER User user() identified by '111111';
create database acl_test_db;

use acl_test_db;
create user acl_user identified by '111111';

#INSERT INTO User (User, Host, authentication_string, ssl_cipher, x509_issuer, x509_subject) VALUES ('acl_user', '10.%', PASSWORD('111111'), '','','');
#INSERT INTO User (User, Host, authentication_string, ssl_cipher, x509_issuer, x509_subject) VALUES ('acl_user', 'localhost', PASSWORD('111111'), '', '', '');

grant CREATE,DROP,INSERT,DELETE,UPDATE,SELECT on acl_test_db.* to 'acl_user';
flush privileges;
