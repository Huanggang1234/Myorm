```c++

#include "myorm.h"

//myorm.h文件开始定义了宏MYORM_SHOW_STETMENT,可以在执行query时打印实际执行的sql语句
//方便查看错误和debug，如不需要可将myorm.h中该宏注释或者删除
//
//该库执行语句的唯一方式时调用myorm::connection::query，其他的结构体为辅助生成sql语句
//myorm::connection::query使用互斥锁保证查询和获取结果为原子的，线程安全，且connection无法拷贝复制
//只能通过std::move()复制
//
//可以使用connection::startTransaction()开始事务模式
//使用connection::rollback()回滚
//使用connection::commit()提交当前事务
//使用connection::endTransaction()结束事务模式
//
//!!!connection::startTransaction()和connection::endTransaction()之间的多个query不保证线程安全
//只保证单个connection::query线程安全
//
//
//
//
//定义一个结构体，对应表字段，顺序应同创建模式时的顺序
struct Node{
   std::string name;
   int  id;
   myorm::optional<double> weight;
   myorm::optional<myorm::date> birth;
   myorm::optional<std::string> intro;
};

//使用DEFINE_STRUCT和DEFINE_FIELD的宏建立反射，DEFINE_STRUCT第一个参数为要反射的结构体名称
//DEFINE_FIELD的第一个参数为结构体的字段名，第二个参数为数据表中的名称
//DEFINE_FIELD的声明顺序，应和创建数据表时相同
//这两个反射宏不能定义在任何namespace内
DEFINE_STRUCT(
   Node,
   DEFINE_FIELD(name,"name"),
   DEFINE_FIELD(id,"id"),
   DEFINE_FIELD(weight,"weight"),
   DEFINE_FIELD(birth,"birth"),
   DEFINE_FIELD(intro,"intro")
)


int main(){
    
    /*以下程序假设数据库中存在表test，如下声明
     *
       create table test(

           name      varchar(15) not null,
	   id                int not null,
           weight         double         ,
	   birth            date         ,
	   intro        tinytext         ,
	   primary key(id)
       )
     *
    */

    //创建一个连接，最后一个参数false可省略，代表立刻创建连接，不用再调用open()
    myorm::connection conn(myorm::connect_options("localhost","root","Hg@200258","hg_test"),false);

    conn.open();

    //如果连接建立成功则为true，否则可以通过打印conn.error()查看具体原因
    if(conn){

	std::cout<<"connect ok"<<std::endl;

/**********************part 1 start******************************/
//通过schema定义一个模式，调用stmt()生成创建语句
//如果返回值res为true则创建成功,否则可以打印res.error()查看原因

        myorm::schema sa("testx");
	sa["name"]="varchar(15) not null";
	sa["id"]="int not null",
	sa["weight"]="double";
	sa["birth"]="date";
	sa["intro"]="tinytext";
        sa.setKey("id");

        auto&& res=conn.query(sa.stmt());
/*********************part 1 end******************************/

/*************************************************************/
//创建一些值，可以不用设置optional类型的成员，这样在插入时将会自动设置成null

        Node v1,v2,v3;

        v1.name="Kai";
	v2.name="lvy";
	v3.name="Leo";
        v1.id=21584763;
	v2.id=75930214;
	v3.id=63897412;
	v1.weight=55.43;
	v2.birth="2001-11-5";
	v3.intro="I\\' m Leo";

/**************************************************************/

//创建一个表对象，便于调用增删改查
//实际也可以以静态的方式调用，直接在对应方法前加上表名参数
//例如myorm::table<Node>::insert("test",v1);

//	myorm::table<Node> tb("test");
 
//单个值插入
//	auto&& res=conn.query(tb.insert(v1));
//
//多值插入
//      auto&& res=conn.query(tb.insert({v2,v3}));
//
//删除记录，参数实际代表where条件
//      auto&& res=conn.query(tb.remove("id=21584763"));
//
//修改记录单个字段，参数1为字段名，参数2为值，最后一个参数      
//      auto&& res=conn.query(tb.update("weight",22.7,"id=21584763"));
//
//同时修改多个字段，参数1为为要修改的字段名数组，参数2为用VALUES宏包裹的要修改成的值，末尾为where条件
//      auto&& res=conn.query(tb.update({"weight","birth"},VALUES(63.5,myorm::date("2010-9-25")),"id=90372618"));  
//
//查询所有字段     
//      auto&& res=conn.query(tb.select());
//
//查询部分字段，后可加where
//      auto&& res=conn.query(tb.select({"id","weight"}));
//
//+where,只能在{}后使用
//      auto&& res=conn.query(tb.select({"id","weight"},"id=21584763"));


	if(res){
            std::cout<<"query succeed"<<std::endl;

//以下为3种对select的结果集进行遍历的方式

/********************** 1 start ************************************/
/*
            while(!res.eof()){
                std::string name;
		int id;
		myorm::optional<double> weight;
                myorm::optional<myorm::date> birth;
		myorm::optional<std::string> intro;

//先声明想要获取的变量
//然后通过fetch函数绑定，参数数量不同
//对应位置的变量，对对应位置的select的字段应该是类型兼容的
//
//              res.fetch(name,id,weight,birth,intro);
//
//不用全部获取，但左侧的值必须被获取              
                res.fetch(id,weight);              
                
		std::cout<<"name:"<<name<<" id:"<<id<<" weight:"<<weight<<" birth:"<<birth<<" intro:"<<intro<<std::endl;

		res.next();
	    }
*/
/***********************1 end ************************************/


/*********************** 2 start ************************************/
/*  
//通过get_table方法获取Node结构体组成的vector数组

	    auto&& vec=res.get_table<Node>();

            for(auto& v:vec){
                std::cout<<"name:"<<v.name<<" id:"<<v.id<<" weight:"<<v.weight<<" birth:"<<v.birth<<" intro:"<<v.intro<<std::endl;
	    }
*/
/************************2 end ************************************/



/************************3 start************************************/
/*
//对结果集res传入一个遍历使用的可调用对象，这将对数据集的每行应用该函数
//这里传入的是lambda表达式，但也可以是函数指针或者std::function<xxx>
//
	    res.each([](int& id,myorm::optional<double>& weight){
                         std::cout<<"id:"<<id<<" weight:"<<weight<<std::endl;
		    });
*/	
/************************3 end ************************************/

	}else{
            std::cout<<"error:"<<res.error()<<std::endl;
	}

    }else{
        std::cout<<"error:"<<conn.error()<<std::endl;
    }

    return 0;
}

/******************************************************
//更多用法请查看研究源码头文件，src only 1500 
 *****************************************************/





```
