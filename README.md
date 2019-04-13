# bookshelfdq

## 介绍
### 你是否需要它？ 
* 图书太多,不知道放到哪个书架了？
* 书架太大,外侧的书挡住里侧的书？
* 你的书架可能只是个柜子？

换句话说，当你不能一瞬间找到你的书时，你可能需要它的协助...
    
* 找不到小巧的图书管理软件?
* 想要按照自己的意愿给图书打上标签或者简短的心得?

简单的说，如果你需要 `小型图书管理器+一些人性化的管理方式` ,那么你可以尝试它...

* 支持 `5` 个用户远程同时登录,每个用户独立存储 `1w` 本书

如果你需要以上这些功能...请继续往下读...

### 它是什么?
    就像这样：
![](https://github.com/DaQuiTree/bookshelfdq/raw/master/PICS/manpage.png)

    你可以管理你所有的书架，像这样:
![](https://github.com/DaQuiTree/bookshelfdq/raw/master/PICS/shelfpage.png)

    当然，事先你必须打造你的书架, 像这样:
![](https://github.com/DaQuiTree/bookshelfdq/raw/master/PICS/buildpage.png)<br>

    也可以管理你所有的图书，像这样:
![](https://github.com/DaQuiTree/bookshelfdq/raw/master/PICS/bookpage.png)<br>

    或检索你的图书,像这样:
![](https://github.com/DaQuiTree/bookshelfdq/raw/master/PICS/seachpage.png)<br>

    更重要的是,别忘了你的密码:
![](https://github.com/DaQuiTree/bookshelfdq/raw/master/PICS/login.png)<br>

### 它的优点?
  * 安全
    * 客户端注册账户时做了基于词典的密码强度设置
    * 存储用户密码时做了HASH加密处理
    * TCP/IP通讯做了基于openssl的加密处理
  
  * 高效 
    * 轻量级服务器程序，简单快速处理客户端请求
    * 客户端基于dbm数据库的缓存机制，减轻服务器压力
    
### 它的缺点?(disadvantage)
  * Temporarily only supports Chinese usage environment.
  * 使用UTF8编码，删除中文字符时需要点击三次Backspace,否则可能出现乱码现象
 
## 安装

### 服务器端 (以centos 6为例)
  #### 安装依赖文件
  1. openssl
     
    sudo yum install openssl openssl-devel
    
  2. mysql 5.7 
    参考：<br>
    [centos6 安装 mysql 5.7](https://opensourcedbms.com/dbms/installing-mysql-5-7-on-centosredhatfedora/)    
    [mysql中文支持](https://blog.csdn.net/u012410733/article/details/61619656)
    
  #### 编译安装
    cd bsdqserver
    make && sudo make install 
   可执行文件bsdq_server会安装到/usr/local/bin下
   
 #### 使用bsdq_server
   1. 启动mysql并创建用户bsdq及数据库bsdq_db
      
    /etc/init.d/mysqld start
    mysql -u root -p
    create database bsdq_db;
    GRANT ALL ON bsdq_db.* TO bsdq@localhost IDENTYFIED BY 'yourpassword';
    
  2. 修改bsdq_server管理员密码:(初始密码000000)
  
    bsdq_server --admin-reset
    
  3. 启动bsdq_server
  
    bsdq_server [-p] [password]
   
### 客户端 (Ubuntu 16.04为例)
   #### 安装依赖库
   1. mysqlclient以及开发包
   
    sudo apt install libmysqlclient-dev libmysql++-dev
    
   2. ncurses开发包
   
    sudo apt install libncursesw5-dev
   
   3. dbm数据库开发包
    
    sudo apt install libncursesw-dev
    
   4. openssl开发包
    
    sudo apt install libssl-dev
    
   5. ldap开发包
    
    sudo apt install slapd ldap-utils
   
   #### 安装依赖文件
   1. LDAP相关头文件
   
    cd bsdqclient/before_install
    sudo cp -r openldap-2.4.47 /usr/local/
    
   2. cracklib-2.9.7

    cd bsdqclient/before_install/cracklib-2.9.7
    autoreconf -f -i
    ./config
    make && sudo make install
    
   #### 编译生成可执行文件bookMan
  
   注1：如果bsdq_server运行在远程服务器上，请修改clisrv.h中SERVER_HOSTNAME宏为远端服务器地址
    
   例如：
   
    #define SERVER_HOSTNAME (130.120.196.121)
    
   然后执行：
    
    cd bsdqclient/ 
    make && sudo make install
    
   注2:编译时如遇到无法找到lldap_r和llber库，需手动执行:
   
    sudo ln -s 路径/liblber-2.4.so.2 路径/liblber.so
    sudo ln -s 路径/libldap_r-2.4.so.2 路径/libldap_r.so
    
   #### 使用bookMan
   
   1.准备工作(复制对称加密文件)
   
    若bsdq_server在远程服务器上，请将服务器端/usr/local/bookshelfdq/.aes_key.config文件复制到本机
   
   2.shell下执行bookMan，开始管理你的图书吧！
    
    
   
