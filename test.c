#include<stdio.h>
#include<mysql/mysql.h>



int main(){

    MYSQL mysql;
    
    mysql_init(&mysql);

    if(!mysql_real_connect(&mysql,"localhost","root","Hg@200258","hg_test",0,NULL,0)){
 
         printf("error:%s\n",mysql_error(&mysql));

    
    }else{
    
         
	 printf("connect ok\n");
    
    
    }
  
    mysql_query(&mysql,"select * from test_table");

    MYSQL_RES *res=mysql_store_result(&mysql);//如果返回值为NULL说明发生了错误，而不是空集，空集有对应的查询结果而不是NULL


    printf("列数:调用1：%u 调用2：%u\n",mysql_field_count(&mysql),mysql_num_fields(res));//返回列数，同时可以检测是否返回空集
    printf("行数:%llu\n",mysql_num_rows(res));

    unsigned int field_num=mysql_num_fields(res);
    unsigned int row_num=mysql_num_rows(res);

    MYSQL_ROW row=mysql_fetch_row(res);


    for(;row!=NULL;row=mysql_fetch_row(res)){

        unsigned long * lengths=mysql_fetch_lengths(res);//获取每一列字段的长度，返回值为一个数组

        for(int i=1;i<=field_num;i++){
      
           printf("列%d:%lu: ",i,lengths[i-1]);

           for(int j=0;j<lengths[i-1];j++)
	     putchar(row[i-1][j]);
          
           printf("      ");


        }
        printf("\n");
    }


    mysql_free_result(res);//释放查询缓存


    mysql_library_end();//无参函数

   
    return 0;
}







