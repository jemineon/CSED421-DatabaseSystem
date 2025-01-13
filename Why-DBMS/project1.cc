// Copyright 2022 Wook-shin Han
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include "./project1.h"
// using namespace std;

struct Customer {
  std::string UNAME;
  std::string PASSWD;
  std::string LNAME;
  std::string FNAME;
  std::string ADDR;
  int ZONE;
  char SEX;
  int AGE;
  int LIMIT;
  float BALANCE;
  int CREDITCARD;
  std::string EMAIL;
  int ACTIVE;
};

struct Zonecost {
  int ZONEID;
  std::string ZONEDESC;
};

struct Lineitem {
  std::string BARCODE;
  int QUANTITY;
  float PRICE;
};

struct Product {
  std::string BARCODE;
  std::string PROD;
  int buy_num;
};

int main(int argc, char** argv) {
  /* fill this */

  std::string q = argv[1];
  std::string line;

  // q1
  int customer_count[13][2];
  int zonecost_count[3][2];

  // q2
  int lineitem_count[6][2];
  int product_count[8][2];

  std::vector<int> v;
  int v_cnt = 0;
  if (q == "q1") {
    std::ifstream customer(argv[2]);
    std::ifstream zonecost(argv[3]);
    struct Customer customer_list[10];  // ??
    struct Zonecost zonecost_list[10];
    int customer_cnt = 0;
    int zonecost_cnt = 0;
    // std::string result[10];

    if (customer.is_open()) {
      while (getline(customer, line)) {
        if (line[0] == 'U') {
          // std::cout << line <<"\n" ;
          continue;
        }

        if (line[0] == '-') {
          int pos = 0;
          int i = 0;
          int pos_nxt;
          while ((pos_nxt = line.find(" ", pos)) != std::string::npos) {
            customer_count[i][0] = pos;
            customer_count[i][1] = pos_nxt - pos;
            pos = pos_nxt + 1;
            i++;
          }
          customer_count[12][0] = pos;
          customer_count[12][1] = 1;

        } else {
          // 2 LNAME, 5 ZONE, 12 ACTIVE

          customer_list[customer_cnt].LNAME =
              line.substr(customer_count[2][0], customer_count[2][1]);
          std::string str_zone;
          str_zone = line.substr(customer_count[5][0], customer_count[5][1]);
          int num_zone = str_zone.find(" ");
          str_zone = str_zone.substr(0, num_zone);

          // customer_list[customer_cnt].ZONE =
          // stoi(line.substr(customer_count[5][0], customer_count[5][1]));
          customer_list[customer_cnt].ZONE = stoi(str_zone);
          // std::cout<<"abcde\n";
          customer_list[customer_cnt].ACTIVE =
              stoi(line.substr(customer_count[12][0], customer_count[12][1]));
          // std::cout<<"AAAAA\n";
          customer_cnt++;
        }
      }
      customer.close();
    } else {
      std::cout << "customer open error";
      return 1;
    }

    if (zonecost.is_open()) {
      while (getline(zonecost, line)) {
        // std::cout<< line <<"\n";
        if (line[0] == 'Z') {
          // std::cout<< line <<"\n";
          continue;
        }
        if (line[0] == '-') {
          int pos = 0;
          int i = 0;
          int pos_nxt;
          while ((pos_nxt = line.find(" ", pos)) != std::string::npos) {
            zonecost_count[i][0] = pos;
            zonecost_count[i][1] = pos_nxt - pos;
            pos = pos_nxt + 1;
            i++;
          }
        } else {
          // 2 LNAME, 5 ZONE, 12 ACTIVE
          std::string str_zoneid;
          // std::cout<<"BBB";
          str_zoneid = line.substr(zonecost_count[0][0], zonecost_count[0][1]);
          // std::cout<<str_zoneid <<"HERE\n";
          int num_zone = str_zoneid.find(" ");
          str_zoneid = str_zoneid.substr(0, num_zone);
          // std::cout<<"BBB";
          zonecost_list[zonecost_cnt].ZONEID = stoi(str_zoneid);

          zonecost_list[zonecost_cnt].ZONEDESC =
              line.substr(zonecost_count[1][0], zonecost_count[1][1]);
          zonecost_cnt++;
          // std::cout<<"abcde";
        }
      }
      zonecost.close();
    } else {
      std::cout << "zonecost open error";
      return 1;
    }

    int zoneid = -1;
    for (int i = 0; i < zonecost_cnt; i++) {
      if (zonecost_list[i].ZONEDESC.substr(0, 7) == "Toronto") {
        // std::cout<<"abcde";
        v.push_back(zonecost_list[i].ZONEID);
        v_cnt++;
      }
    }

    // ofstream result(argv[4]);
    // ofstream result(argv[5]);
    std::ofstream result("temp.txt");
    if (result.is_open()) {
      for (int i = 0; i < customer_cnt; i++) {
        for (int j = 0; j < v_cnt; j++) {
          zoneid = v[j];
          if (customer_list[i].ZONE == zoneid && customer_list[i].ACTIVE == 1) {
            // std::string s = strtok(customer_list[i].LNAME, " ");

            result << customer_list[i].LNAME << "\n";
          }
        }
      }
      result.close();
    }
  }

  if (q == "q2") {
    std::ifstream lineitem(argv[2]);
    std::ifstream product(argv[3]);
    struct Lineitem lineitem_list[40];  // ??
    struct Product product_list[40];
    int product_cnt = 0;
    int lineitem_cnt = 0;

    if (lineitem.is_open()) {
      while (getline(lineitem, line)) {
        if (line[0] == 'U') {
          continue;
        }
        if (line[0] == '-') {
          int pos = 0;
          int i = 0;
          int pos_nxt;
          while ((pos_nxt = line.find(" ", pos)) != std::string::npos) {
            lineitem_count[i][0] = pos;
            lineitem_count[i][1] = pos_nxt - pos;
            pos = pos_nxt + 1;
            i++;
          }
          lineitem_count[5][0] = pos;
          lineitem_count[5][1] = 1;
        } else {
          // 3 BARCODE

          std::string str_barcode;
          str_barcode = line.substr(lineitem_count[3][0], lineitem_count[3][1]);
          int num_barcode = str_barcode.find(" ");
          str_barcode = str_barcode.substr(0, num_barcode);

          // customer_list[customer_cnt].ZONE =
          // stoi(line.substr(customer_count[5][0], customer_count[5][1]));
          lineitem_list[lineitem_cnt].BARCODE = (str_barcode);
          lineitem_cnt++;
        }
      }
      lineitem.close();

    } else {
      std::cout << "lineitem open error";
      return 1;
    }

    if (product.is_open()) {
      while (getline(product, line)) {
        if (line[0] == 'U') {
          continue;
        }
        if (line[0] == '-') {
          int pos = 0;
          int i = 0;
          int pos_nxt;
          while ((pos_nxt = line.find(" ", pos)) != std::string::npos) {
            product_count[i][0] = pos;
            product_count[i][1] = pos_nxt - pos;
            pos = pos_nxt + 1;
            i++;
          }
          product_count[7][0] = pos;
          product_count[7][1] = 1;
        } else {
          // 0 BARCODE 2 PRODESC
          std::string str_barcode;
          str_barcode = line.substr(product_count[0][0], product_count[0][1]);
          int num_barcode = str_barcode.find(" ");
          str_barcode = str_barcode.substr(0, num_barcode);

          // customer_list[customer_cnt].ZONE =
          // stoi(line.substr(customer_count[5][0], customer_count[5][1]));
          product_list[product_cnt].BARCODE = (str_barcode);
          product_list[product_cnt].PROD =
              line.substr(product_count[2][0], product_count[2][1]);
          product_cnt++;
        }
      }
      product.close();
    } else {
      std::cout << "product open error";
      return 1;
    }

    // product들 buy num 초기화
    for (int i = 0; i < product_cnt; i++) {
      product_list[i].buy_num = 0;
    }
    // product들 buy num 설정
    for (int i = 0; i < product_cnt; i++) {
      for (int j = 0; j < lineitem_cnt; j++) {
        if (product_list[i].BARCODE == lineitem_list[j].BARCODE) {
          product_list[i].buy_num++;
        }
      }
    }

    std::ofstream result("temp.txt");
    // ofstream result(argv[5]);
    if (result.is_open()) {
      for (int i = 0; i < product_cnt; i++) {
        if (product_list[i].buy_num >= 2) {
          result << product_list[i].BARCODE << "                 "
                 << product_list[i].PROD << "\n";
        }
      }
      result.close();
    }

    return 0;
  }

  return 0;
}
