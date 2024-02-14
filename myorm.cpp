#ifndef _MYORM_H_
#define _MYORM_H_


#include<mysql/mysql.h>
#include<string>
#include<cstring>
#include<cstdarg>
#include<vector>
#include<cstdint>
#include<cstddef>
#include<exception>
#include<tuple>
#include<utility>
#include<type_traits>
#include<functional>
#include<iostream>
#include<sstream>

namespace myorm{

     template<typename T>
     struct no_const_and_reference{
          using type=T;
     };

     template<typename T>
     struct no_const_and_reference<T&>{
	  using type=T;
     };

     template<typename T>
     struct no_const_and_reference<T&&>{
          using type=T;
     };

     template<typename T>
     struct no_const_and_reference<const T&>{
          using type=T;
     };

     template<typename T>
     struct no_const_and_reference<const T&&>{
          using type=T;
     };
 
     template<typename Function>
     struct function_trait;

     template<typename Ret,typename... Args>
     struct function_trait<Ret(Args...)>{

          using return_type=Ret;
          using args_type=std::tuple<Args...>;
	  const static int args_num=sizeof...(Args);

          template<int I>
	  struct argument{
               using type=typename std::tuple_element<I,args_type>::type;
	  };
          
	  template<int I>
	  using nth_argument_type=typename argument<I>::type;

          template<int I>
          using nth_raw_type=typename no_const_and_reference<nth_argument_type<I>>::type;

	  using raw_return_type=typename no_const_and_reference<return_type>::type;
     };

     template<typename Ret,typename... Args>
     struct function_trait<Ret(*)(Args...)>:public function_trait<Ret(Args...)>{};

     template<typename Ret,typename... Args>
     struct function_trait<std::function<Ret(Args...)>>:public function_trait<Ret(Args...)>{};

     template<typename C,typename Ret,typename... Args>
     struct function_trait<Ret(C::*)(Args...)>:public function_trait<Ret(Args...)>{};

     //以下用于处理lambda函数
     template<typename C,typename Ret,typename... Args>
     struct function_trait<Ret(C::*)(Args...) const>:public function_trait<Ret(Args...)>{};

     template<typename Function>
     struct function_trait{

         using lambda=function_trait<decltype(&Function::operator())>;
	 using return_type=typename lambda::return_type;
	 using args_type=typename lambda::args_type;
	 const static int args_num=lambda::args_num;

	 template<int I>
         using nth_argument_type=typename lambda::template nth_argument_type<I>;

	 template<int I>
	 using nth_raw_type=typename lambda::template nth_raw_type<I>;

         using raw_return_type=typename lambda::raw_return_type;
     };

     class __null_value{};
     const __null_value null_value;

     template<typename T>
     class optional{

           private:
	       bool no_value;
	       T value;
	   public:

               optional():no_value(true){}

	       template<typename... Args>
	       optional(Args... args):no_value(false),value(args...){}

               const T& operator*(){
                   return value;
	       }

               const optional& operator=(const optional& _value){
                     no_value=_value.no_value;
		     value=_value.value;
		     return *this;
	       }


	       const T& operator=(const T& _value){
                    no_value=false;
		    value=_value;
		    return value;
	       }


               const T& operator=(const T&& _value){
                    no_value=false;
		    value=std::move(_value);
		    return value;
	       }

               const __null_value& operator=(const __null_value& _null_value){
                    no_value=true;
		    return _null_value;
	       }

               operator bool(){
                    return !no_value;
	       }

     };


     template<>
     class optional<std::string>{

           private:
	       bool no_value;
               std::string value;
	   public:

	       optional():no_value(true){}

               template<typename... Args>
	       optional(Args... args):no_value(false),value(args...){}


	       const std::string& operator*(){
                     return value;
	       }
              
               const optional<std::string>& operator=(const optional<std::string>& _value){
                     no_value=_value.no_value;
		     value=_value.value;
		     return *this;
	       }

               const std::string& operator=(const std::string& _value){
                     no_value=false;
		     value=_value;
		     return value;
	       }

               const std::string& operator=(const std::string&& _value){
                     no_value=false;
		     value=std::move(_value);
		     return value;
	       }

               const std::string& operator=(const char* _value){
                     no_value=false;
		     value=_value;
		     return value;
	       }

	       const __null_value& operator=(const __null_value& _null_value){
                     no_value=true;
		     return _null_value;
	       }

	       operator bool() const{
                    return !no_value;
	       }
     };

     template<typename T>
     struct is_option:public std::false_type{};

     template<typename T>
     struct is_option<optional<T>>:public std::true_type{};

     template<typename T>
     struct is_option<optional<T>&&>:public std::true_type{};

     template<typename T>
     struct is_option<optional<T>&>:public std::true_type{};

     template<typename T>
     std::stringstream& operator<<(std::stringstream&sm,optional<T>& value){
             if(value)
	        sm<<*value;
	      return sm;
     }

     template<typename T>
     std::ostream& operator<<(std::ostream&om,optional<T>& value){
             if(value)
	        om<<*value;
	     return om;
     }


     template<int I,int All,typename Tuple,typename Function>
     typename std::enable_if<(I==All),void>::type
     eachTuple(Tuple&& tp,Function&& fun){}

     template<int I,int All,typename Tuple,typename Function>
     typename std::enable_if<(I<All),void>::type
     eachTuple(Tuple&& tp,Function&& fun){

        using nf_type=typename std::remove_reference<Tuple>::type;
	using type=typename std::tuple_element<I,nf_type>::type;
        auto&& value=std::get<I>(tp);
	fun(std::forward<decltype(value)>(value),I);
	eachTuple<I+1,All>(std::forward<Tuple&&>(tp),std::forward<Function&&>(fun));
     }

     template<typename T>
     inline constexpr auto StructSchema(){
         return std::make_tuple();
     }

     template<typename T,typename Fn>
     inline constexpr void ForEachField(T&&value,Fn&&fn){

	 constexpr auto struct_schema=StructSchema<std::decay_t<T>>();
         eachTuple<0,std::tuple_size<decltype(struct_schema)>::value>(struct_schema,[&value,&fn]
         (auto&field_schema,int index){
             fn(value.*(std::get<0>(std::forward<decltype(field_schema)>(field_schema))),
		std::get<1>(std::forward<decltype(field_schema)>(field_schema)),index);
	 });
     }


#define DEFINE_STRUCT(Struct,...) \
namespace myorm{ \
template<> \
inline constexpr auto StructSchema<Struct>(){ \
      using _Struct = Struct; \
      return std::make_tuple(__VA_ARGS__); \
} }

#define DEFINE_FIELD(StructField,FieldName) \
 std::make_tuple(&_Struct::StructField,FieldName)


     std::string format_string(const char* format,...){

         uint32_t size=256;
         std::vector<char> buf(size);

         va_list argv;
	 va_start(argv,format);
         
	 uint32_t count;

         count=vsnprintf(&buf[0],size,format,argv);
      
	 va_end(argv);

         return &buf[0];

     }

     class mysql_exception:public std::exception {

           private:
              std::string msg;

	   public: 
	      mysql_exception(const char* message):msg(message){}

           const char* what() const noexcept override {

              return msg.c_str();

	   }
     };

     
    
     struct connect_options;
     class connection;
     class result;


     struct connect_options{

	  std::string   server;
	  std::string   user;
	  std::string   password;
	  std::string   dbname;
	  std::string   charset;
          unsigned int  port;

	  connect_options(
               const std::string& _server="",
               const std::string& _user="",
	       const std::string& _password="",
	       const std::string& _dbname="",
               const std::string& _charset="",
	       unsigned int _port=0
          ):
	      server(_server),
	      user(_user),
	      password(_password),
	      dbname(_dbname),
	      charset(_charset),
              port(_port)
          {}

     };



     class result{

           public:

	      template<typename... Args>
              class iterator{
                 private:
                    uint64_t index;
		    result*  res;
		    std::tuple<Args...> data;

	      };

           private:
              MYSQL* conn=nullptr;
	      std::vector<std::string> field_name;
	      std::vector<enum_field_types> field_type;
              std::vector<std::vector<std::string>> rows;
              std::vector<std::vector<std::string>>::iterator current_row_itr;
              bool fetched=false;
	      uint32_t  fields_num=0;

              template<typename Function,typename... Args>
              typename std::enable_if<(sizeof...(Args)<function_trait<Function>::args_num),void>::type
              bind_and_call(Function&& callback,Args&&... args){
                   
		  typename function_trait<Function>::template nth_raw_type<sizeof...(Args)> value;
                  get_value(sizeof...(Args),value);
                  bind_and_call(std::forward<Function&&>(callback),std::forward<Args&&>(args)...,std::move(value));
	      }

              template<typename Function,typename... Args>
              typename std::enable_if<(sizeof...(Args)==function_trait<Function>::args_num),void>::type
              bind_and_call(Function&& callback,Args&&... args){
                   callback(args...);
	      }



           public:

              result(MYSQL* _conn, bool _fetch_now=false):conn(_conn){

		  if(_fetch_now)fetch();
	      }

              ~result(){

                  if(!fetched)fetch();
                  field_name.clear();
		  field_type.clear();
                  rows.clear();
	      }
         
              void check(){
                   if(!fetched)fetch();
	      }

              void fetch(){

                    if(fetched) {
			throw mysql_exception("already fetched");
		    }
                   
		    MYSQL_RES *res=mysql_store_result(conn);
                    
		    fields_num=mysql_num_fields(res);
		    auto fields=mysql_fetch_fields(res);

                    if(res!=nullptr){

                      for(MYSQL_ROW row=mysql_fetch_row(res);row!=nullptr;row=mysql_fetch_row(res)){

			   unsigned long* lengths=mysql_fetch_lengths(res);

			   if(!lengths) throw mysql_exception(mysql_error(conn));
                           
                           std::vector<std::string> rrow;

			   for(int i=0;i<fields_num;i++){
                               rrow.push_back(std::string(row[i],lengths[i]));
			   }

			   rows.push_back(std::move(rrow));
		      }
                      
                      for(int i=0;i<fields_num;i++){
                           field_name.push_back(std::string(fields[i].org_name,fields[i].org_name_length));
                           field_type.push_back(fields[i].type);
		      }

                      fetched=true;
		      current_row_itr=rows.begin();

                      mysql_free_result(res);      

		    }else{

                      throw mysql_exception(mysql_error(conn));

		    }
	      }


	      uint64_t count(){
		  check();
		  return rows.size();
	      }

	      uint32_t field(){
                  check();
		  return fields_num;
	      }

              const std::string& field_name_at(int index){
                  return field_name[index];
	      }

              const enum_field_types& field_type_at(int index){
                  return field_type[index];
	      }


              bool eof(){
		   check();
                   return current_row_itr==rows.end();
	      }
	      
              
	      void next(){
		   check();
                   current_row_itr++;
	      }

              void prev(){
		   check();
                   current_row_itr--;
	      }

              uint64_t tell(){

		   check();
                   return current_row_itr-rows.begin();
	      }

              void seek(uint64_t offset){

		   check();
                   current_row_itr=rows.begin()+offset;
	      }


              const char* fetch_field_data(uint32_t index){
		    check();
                    return (*current_row_itr)[index].c_str();
	      }

              const std::vector<std::vector<std::string>>& res(){

		    return rows;

	      }

              template<typename T>
	      bool get_value(int i,optional<T>& value){
 
                   T _value;

		   if(get_value(i,_value)){
                      
		      value=std::move(_value);
		      return true;
	           }else{
                      value=null_value;
                      return false;
		   }
	      }


              bool get_value(int i,bool& value){

		   const char* s=fetch_field_data(i);

                   if(s==nullptr)return false;

		   try{

                       value=(std::stoi(s)!=0);
                       return true;

		   }catch(std::exception&){

                       return false;
		   }
	      }

              bool get_value(int i,int& value){

                   const char* s=fetch_field_data(i);
                   
		   if(s==nullptr)return false;

		   try{

                           value=std::stoi(s);
			   return true;

		   }catch(std::exception&){
                           return false;
		   }

	      }

              bool get_value(int i,unsigned int& value){

                   const char* s=fetch_field_data(i);
                   
		   if(s==nullptr)return false;

		   try{

                           value=(unsigned int)std::stoul(s);
			   return true;

		   }catch(std::exception&){
                           return false;
		   }

	      }

              bool get_value(int i,long& value){

                   const char* s=fetch_field_data(i);
                   
		   if(s==nullptr)return false;

		   try{

                           value=std::stol(s);
			   return true;

		   }catch(std::exception&){
                           return false;
		   }

	      }

              bool get_value(int i,unsigned long& value){

                   const char* s=fetch_field_data(i);
                   
		   if(s==nullptr)return false;

		   try{

                           value=std::stoul(s);
			   return true;

		   }catch(std::exception&){
                           return false;
		   }
	      }

              bool get_value(int i,long long& value){

                   const char* s=fetch_field_data(i);
                   
		   if(s==nullptr)return false;

		   try{

                           value=std::stoll(s);
			   return true;

		   }catch(std::exception&){
                           return false;
		   }

	      }

              bool get_value(int i,unsigned long long& value){

                   const char* s=fetch_field_data(i);
                   
		   if(s==nullptr)return false;

		   try{

                           value=std::stoull(s);
			   return true;

		   }catch(std::exception&){
                           return false;
		   }

	      }

              bool get_value(int i,float& value){

                   const char* s=fetch_field_data(i);
                   
		   if(s==nullptr)return false;

		   try{

                           value=std::stof(s);
			   return true;

		   }catch(std::exception&){
                           return false;
		   }

	      }

              bool get_value(int i,double& value){

                   const char* s=fetch_field_data(i);
                   
		   if(s==nullptr)return false;

		   try{

                           value=std::stod(s);
			   return true;

		   }catch(std::exception&){
                           return false;
		   }

	      }

              bool get_value(int i,long double& value){

                   const char* s=fetch_field_data(i);
                   
		   if(s==nullptr)return false;

		   try{

                           value=std::stold(s);
			   return true;

		   }catch(std::exception&){
                           return false;
		   }

	      }

              bool get_value(int i,std::string& value){

                   const char* s=fetch_field_data(i);
                   
		   if(s==nullptr)return false;

		   try{

                           value=s;
			   return true;

		   }catch(std::exception&){
                           return false;
		   }

	      }

              template<int All,int I,typename... Args>
              typename std::enable_if<(All==I),void>::type
              do_fetch(Args&&... args){}

	      template<int All,int I,typename T,typename... Args>
	      typename std::enable_if<(All>I),void>::type
	      do_fetch(T&& value,Args&&... args){
 
		   get_value(I,value);
                   do_fetch<All,I+1>(std::forward<Args&&>(args)...);
	      }

              template<typename... Args>
	      void fetch(Args&&... args){//加上&&是为了让Args推导成引用类型，而不是普通类型
                   do_fetch<sizeof...(Args),0>(std::forward<Args&&>(args)...);
	      }


              template<typename Function>
              void each(Function&& callback){
                   
		  seek(0);

		  while(!eof()){
                     bind_and_call(std::forward<Function&&>(callback));
		     next();
		  }
	      }
     };


     template<typename T>
     class table{

	   private:
	       std::string name;
               std::vector<std::string> field_name;
	       std::vector<enum_field_types> field_type;
	       std::vector<T>  data;
	       connection *conn;
	   public:

	       void insert(T& value){

                    std::ostringstream stmt;
		    constexpr auto struct_schema=StructSchema<std::decay_t<T>>();
	            int num=std::tuple_size<decltype(struct_schema)>::value-1;
                    stmt<<"insert into "<<name<<" ( ";

		    ForEachField(value,[&stmt,num](auto&&field,auto&&name,int index){

                        if(index<num){
                            stmt<<name<<", ";
			}else{
                            stmt<<name<<" ) values ( "; 
			}
	            }); 

		    ForEachField(value,[&stmt,num](auto&&field,auto&&name,int index){

                        if(index<num){
                            stmt<<field<<", ";
			}else{
                            stmt<<field<<" ) values ( "; 
			}
	            }); 

	       }

	       void insert(T&& value);
               void insert(std::vector<T>& values);
               
	       template<int I,typename U>
	       void remove(U&& value);
               
	       template<int I,typename U>
	       void update(U&& value);
	       static void create(connection& conn,std::string&name);
     };

     class connection{

           private:
              MYSQL* conn;

	   public:

	      connection(const connect_options& options){
                 
                   conn=mysql_init(nullptr);
		   open(options);
	      }
             
              ~connection(){
 
		   mysql_close(conn);

	      }

              bool open(const connect_options& options){

		  if(conn==nullptr){

                      return false;

		  }
//mysql_real_connect(MYSQL*,
//                   const char*host,
//                   const char*user,
//                   const char*password,
//                   const char*db,
//                   unsigned int port,
//                   const char* unix_socket,
//                   unsigned int client_flag)
                  conn=mysql_real_connect(conn,options.server.c_str(),options.user.c_str(),options.password.c_str(),options.dbname.c_str(),options.port,nullptr,0);
                  
		  return conn==nullptr;
	      }

              void close(){

                   if(conn==nullptr)return;

                   mysql_close(conn);

		   conn=nullptr;

	      }

              bool is_open() const{
                   
                  if(conn==nullptr)return false;  

                  return mysql_ping(conn)==0;
	      }

              operator bool() const{
                
                   return is_open();

	      }

              result query(const std::string& stmt){

                   mysql_real_query(conn,stmt.c_str(),stmt.length());

		   return result(conn,true);
	      }

	      result query(const std::string&& stmt){

                   mysql_real_query(conn,stmt.c_str(),stmt.length());

		   return result(conn,true);
	      }

              template<typename... Args>
	      result query(const char* format,Args... args){

                   std::string stmt=format_string(format,args...);
		   std::cout<<"the stmt:"<<stmt<<std::endl;
                   return query(stmt);
	      }

              template<typename T>
              std::vector<T> get_table(std::string& name){

                   std::string stmt="select * from "+name;

		   auto res=query(stmt);

                   std::vector<T> rc;

		   int field=res.field();

		   while(!res.eof()){

                       T  node;
                       ForEachField(node,[&res](auto&&field,auto&&name,int index){
                            res.get_value(index,std::forward<decltype(field)>(field));
		       });
                       rc.push_back(std::move(node));
		       res.next();
		   }
		   return std::move(rc);
	     }

             template<typename T>
             std::vector<T> get_table(std::string&& name){
                  return std::move(get_table<T>(name));
	     }
     };

}


void pan(int x){

     if(x<=50){

	throw myorm::mysql_exception("problem\n");

     }
     
     std::cout<<"ok"<<std::endl;

}
    

     void dod(myorm::optional<std::string> &name,myorm::optional<int> &age,myorm::optional<long long>&id,myorm::optional<std::string>& addr){


	 std::cout<<"I'm "<<*name<<", my age is "<<*age<<std::endl;

          if(id){
	      std::cout<<"my id is "<<*id<<std::endl;
	  }else{
	      std::cout<<"I don't have id"<<std::endl;
	  }

	  std::cout<<"my addr is "<<*addr<<std::endl;
     }


struct Node{
    std::string name;
    int age;
    myorm::optional<long long> id;
    std::string addr;
};

DEFINE_STRUCT(
    Node,
    DEFINE_FIELD(name,"name"),
    DEFINE_FIELD(age,"age"),
    DEFINE_FIELD(id,"id"),
    DEFINE_FIELD(addr,"addr")
);


     int main(){


         Node nn;

         if(myorm::is_option<decltype(nn.addr)>::value){
            std::cout<<"YES"<<std::endl;
	 }else{
            std::cout<<"NO"<<std::endl;
	 }



	 myorm::connection conn(myorm::connect_options("localhost","root","Hg@200258","hg_test"));

         if(conn){

/*	     
              auto res=conn.query("select * from test_table");

              for(int i=0;i<res.field();i++){
                   std::cout<<res.field_name_at(i)<<"   ";
	      }
              
              std::cout<<std::endl;

              for(int i=0;i<res.field();i++){
                   std::cout<<res.field_type_at(i)<<"   ";
	      }


	      std::cout<<std::endl;

              res.each(&dod);
*/

              auto table=conn.get_table<Node>("test_table");

	      for(auto& row:table){
                  std::cout<<"name:"<<row.name<<std::endl;
                  std::cout<<"age:"<<row.age<<std::endl;
	//	  if(row.id)
                      std::cout<<"id:"<<row.id<<std::endl;
                  std::cout<<"addr:"<<row.addr<<std::endl;
	      }
	 }

         return 0;
     }

#endif




