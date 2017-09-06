//
//  FileSys.cpp
//  SecondLevelFileSystem
//
//  Created by Eser on 31/08/2017.
//  Copyright © 2017 Crysple. All rights reserved.
//

#include "FileSys.h"

/********** InodeTable *********/
InodeTable::InodeTable(){
    ifstream sysfile(DATAFILE,ios::in|ios::binary);
    sysfile.seekg(0);
    sysfile.read(reinterpret_cast<char*>(entry),INODE_BIT_SIZE);
    sysfile.close();
}

size_t InodeTable::add(Inode * i){
    fstream sysfile(DATAFILE,ios::in|ios::out|ios::binary);
    size_t address=getAvailable()*sizeof(Inode)+INODE_BIT_SIZE;
    sysfile.seekp(address);
    sysfile.write(reinterpret_cast<const char*>(i),sizeof(Inode));
    sysfile.close();
    return address;
}

Inode* InodeTable::getInode(size_t address){
    ifstream sysfile(DATAFILE,ios::in|ios::binary);
    auto res=new Inode();
    sysfile.seekg(address);
    sysfile.read(reinterpret_cast<char*>(res),sizeof(Inode));
    sysfile.close();
    return res;
}

size_t InodeTable::getAvailable(){
    for(size_t i=0;i<200;++i){
        if(entry[i]==false){
        	entry[i]=true;
            write();
        	return i;
        }
    }
    return -1;
}

void InodeTable::release(size_t add,Bitmap* bitmap){
    auto cur = getInode(add);
    for(size_t i=0;i<cur->blockNum;++i){
        bitmap->release(cur->blockAddress[i]);
    }
    size_t index=(add-INODE_BIT_SIZE)/sizeof(Inode);
    if(index<200) entry[index]=false;
    write();
    delete cur;
}

void InodeTable::write(){
    fstream sysfile(DATAFILE,ios::out|ios::in|ios::binary);
    sysfile.seekp(0);
    sysfile.write(reinterpret_cast<const char*>(entry),INODE_BIT_SIZE);
    sysfile.close();
}





/********** Bit map *********/
void Bitmap::init(){
    for(size_t i=0;i<BLOCK_NUM;++i) bitmap[i]=false;
}

void Bitmap::write(){
    fstream sysfile(DATAFILE,ios::out|ios::in|ios::binary);
    sysfile.seekp(INODE_TABLE_SIZE);
    sysfile.write(reinterpret_cast<const char*>(this),BITMAP_SIZE);
    sysfile.close();
}
size_t Bitmap::getNext(){
    for(size_t i=0;i<BLOCK_SIZE;++i){
        if(!bitmap[i]){
        	bitmap[i]=true;
            write();
        	return i*BLOCK_SIZE+INODE_TABLE_SIZE+BITMAP_SIZE;
        }
    }
    return -1;
}

void Bitmap::release(size_t add){
    size_t index=(add-INODE_TABLE_SIZE-BITMAP_SIZE)/BLOCK_SIZE;
    bitmap[index]=false;
    write();
}



/********** User *********/
void User::setUsername(string name){
    size_t len=name.size()<30?name.size():29;
    name.copy(username,len);
    username[len]='\0';
}





/********** File *********/

vector<FileEntry> File::getDir(){
    vector<FileEntry> res;
    FileEntry now;
    string all=readAll();
    string field="";
    int flag=0;
    for(char c:all){
        if(c=='$'){
            switch(flag){
                case FILENAME:
                    field.copy(now.fileName,field.length()<30?field.length():29);
                    break;
                case ADDRESS:
                    now.address=stoi(field);
                    break;
                case PROTECTCODE:
                    now.protectCode=stoi(field);
                    break;
                case LENGTH:
                    now.length=stoi(field);
                    res.push_back(now);
            }
            flag=(flag+1)%4;
            field="";
        }
        else field+=c;
    }
    return res;
}

void File::writeDir(const vector<FileEntry>& dir, size_t inode_address){
    string res="";
    for(auto &i:dir){
        res+=string(i.fileName)+'$';
        res+=to_string(i.address)+'$';
        res+=to_string(i.protectCode)+'$';
        res+=to_string(i.length)+'$';
    }
    write(res,inode_address);
    
}
//将本身i-node重新写入i-node表
void File::writeSelf(size_t address){
    fstream sysfile(DATAFILE,ios::in|ios::out|ios::binary);
    sysfile.seekp(address);
    sysfile.write(reinterpret_cast<const char*>(inode),sizeof(Inode));
    sysfile.close();
}

string File::readAll(){
    size_t len=inode->blockNum;
    string res="";
    char temp[BLOCK_SIZE];
    for(size_t i=0;i<len;++i){
        auto BlockAddress=inode->blockAddress[i];
        ifstream sysfile(DATAFILE,ios::in|ios::binary);
        sysfile.seekg(BlockAddress);
        sysfile.read(temp,BLOCK_SIZE);
        sysfile.close();
        res+=temp;
    }
    return res;
}
//写到当前的i-node拥有的block里面
size_t File::write(const string& towrite,size_t inode_address){
    auto vec = split(towrite);
    if(vec.size()>10) return 0;
    fstream sysfile(DATAFILE,ios::in|ios::out|ios::binary);
    size_t i=0;
    for(;i<vec.size();++i){
        //如果这个块本来就有的了就写进去
        if(i<inode->blockNum) sysfile.seekp(inode->blockAddress[i]);
        else{
            //新的块则从bitmap中分配
            inode->blockAddress[inode->blockNum++]=bitmap.getNext();
            sysfile.seekp(inode->blockAddress[i]);
        }
        sysfile.write(reinterpret_cast<const char*>(vec[i].c_str()),vec[i].length());
    }
    //从bitMap中闲置多余的块
    for(size_t j=i;j<inode->blockNum;++j){
        auto address = inode->blockAddress[j];
        bitmap.release(address);
    }
    inode->blockNum=i;
    sysfile.close();
    writeSelf(inode_address);
    return ((i<1)?0:(i-1))*BLOCK_SIZE+vec[i-1].length();
}

vector<string> File::split(const string& now){
    vector<string> res;
    size_t len=now.length();
    size_t block_num=len/BLOCK_SIZE;
    for(size_t i=0;i<block_num;++i){
        res.push_back(string(now.substr(i*BLOCK_SIZE,BLOCK_SIZE)));
    }
    res.push_back(string(now.substr(BLOCK_SIZE*block_num,now.length()-BLOCK_SIZE*block_num))+"\0");
    return res;
}

void File::readBitmap(){
    ifstream sysfile(DATAFILE,ios::in|ios::binary);
    sysfile.seekg(INODE_TABLE_SIZE);
    sysfile.read(reinterpret_cast<char*>(&bitmap),BITMAP_SIZE);
    sysfile.close();
}



/********** FileSys *********/

FileSys::FileSys(){
    prefix="Computer:";
    ifstream checkFile(MAINFILE);
    if(!checkFile.good()){
        checkFile.close();
        boot();
    }
    else checkFile.close();
}
void FileSys::run(){
    p(prefix);
    string command="";
    while(cin>>command){
        transform(command.begin(),command.end(),command.begin(),::tolower);
        if(command=="login"&&login()) break;
        if(command=="adduser") adduser();
        p(prefix);
    }
    init();
    for(;;){
        p(prefix);
        cin>>command;
        transform(command.begin(),command.end(),command.begin(),::tolower);
        if(command=="dir") printDir();
        else if(command=="create") createFile();
        else if(command=="delete") deleteFile();
        else if(command=="open") openFile();
        else if(command=="read") readFile();
        else if(command=="write") writeFile();
        else if(command=="exit") break;
    }
}

void FileSys::printDir(){
    if(fileEntrys.size()==0) return;
    cout<<left<<setw(15)<<"FileName"<<setw(9)<<"Address"<<setw(15)<<"Protect code"<<setw(10)<<"Length"<<endl;
    for(FileEntry& i:fileEntrys){
        cout<<left<<setw(15)<<i.fileName<<setw(9)<<i.address<<setw(15)<<pcToBinary(i.protectCode)<<setw(10)<<i.length<<endl;
    }
}

void FileSys::enterFileName(){
    p("fileName:");
    cin>>fileName;
}

void FileSys::createFile(){
    enterFileName();
    pln("Protect Code(three-bit integer): ");
    int pc;
    cin>>pc;
    FileEntry newfe;
    auto len=fileName.size();
    len=len<30?len:29;
    fileName.copy(newfe.fileName,len);
    newfe.fileName[len]='\0';
    newfe.protectCode=pc;
    //新的inode
    Inode newinode(0);
    auto address=inodeTable.add(&newinode);
    newfe.address=address;
    fileEntrys.push_back(newfe);
    file.setInode(&rootDir);
    file.writeDir(fileEntrys,user.inodeAddress);
    file.setInode(&newinode);
    file.writeSelf(address);
    file.setInode(NULL);
}

void FileSys::openFile(){
    enterFileName();
    for(size_t i=0;i<fileEntrys.size();++i){
        if(string(fileEntrys[i].fileName)==fileName){
            nowFileEntry=&fileEntrys[i];
            file.setInode(inodeTable.getInode(fileEntrys[i].address));
            pln("Open Successfully");
            return;
        }
    }
    pln("No Such File");
}

void FileSys::deleteFile(){
    enterFileName();
    for(auto i=fileEntrys.begin();i<fileEntrys.end();++i){
        if(string(i->fileName)==fileName){
            fileEntrys.erase(i);
            file.setInode(&rootDir);
            file.writeDir(fileEntrys,user.inodeAddress);
            pln("Delete Successfully");
            return;
        }
    }
    pln("No Such File");
}

void FileSys::closeFile(){
    delete file.getInode();
    file.setInode(NULL);
    nowFileEntry=NULL;
    pln("Close Successfully");
}

void FileSys::readFile(){
    pln(file.readAll());
}

void FileSys::pln(const string& s){cout<<s<<endl;}
void FileSys::p(const string& s){cout<<s;}

void FileSys::writeFile(){
    pln("write here. End with '$EOF'");
    string res="",line="";
    cin.clear();
    cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    for(;;){
        getline(cin,line);
        auto pos=line.find("$EOF");
        if(pos!=string::npos){
            line[pos]='\n';
            line[pos+1]='\0';
            res+=line.substr(0,pos+2);
            break;
        }
        line+='\n';
        res+=line;
    }
    auto len=file.write(res,nowFileEntry->address);
    nowFileEntry->length=len;
    file.setInode(&rootDir);
    file.writeSelf(user.inodeAddress);
    file.writeDir(fileEntrys,user.inodeAddress);
}

void FileSys::boot(){
    ofstream sysfile(MAINFILE,ios::binary);
    size_t i=0;
    sysfile.write(reinterpret_cast<const char*>(&i),sizeof(size_t));
    sysfile.close();
    sysfile.open(DATAFILE,ios::binary);
    bool tempTable[200]={0};
    sysfile.seekp(0);
    sysfile.write(reinterpret_cast<const char*>(tempTable),INODE_BIT_SIZE);
    Bitmap tempBitmap;
    tempBitmap.init();
    sysfile.seekp(INODE_TABLE_SIZE);
    sysfile.write(reinterpret_cast<const char*>(&tempBitmap),BITMAP_SIZE);
    sysfile.close();
    file.readBitmap();
    inodeTable=InodeTable();
}

string FileSys::pcToBinary(int n){
    string res="";
    auto convert=[&res](int i){
        switch(i){
            case 0:res+="000";break;
            case 1:res+="001";break;
            case 2:res+="010";break;
            case 3:res+="011";break;
            case 4:res+="100";break;
            case 5:res+="101";break;
            case 6:res+="110";break;
            case 7:res+="111";break;
        }
    };
    convert(n/100);
    convert(n%100/10);
    convert(n%10);
    return res;
}

bool FileSys::login(){
    string username,password;
    p("Username: ");
    cin>>username;
    p("Password: ");
    cin>>password;
    //读取主文件的用户，进行匹配登录
    ifstream sysfile(MAINFILE,ios::in|ios::binary);
    size_t userNum=0;
    sysfile.read(reinterpret_cast<char *>(&userNum),sizeof(size_t));
    for(size_t i=0;i<userNum;++i){
        sysfile.seekg(sizeof(size_t)+i*sizeof(User));
        sysfile.read(reinterpret_cast<char *>(&user),sizeof(User));
        //匹配成功，直接退出
        if(user.username==username&&user.password==password){
            pln("Login successfully");
            sysfile.close();
            return true;
        }
    }
    sysfile.close();
    //匹配失败，继续登录
    pln("Login Failed");
    return false;
}

void FileSys::adduser(){
    User newUser;
    p("Username:");
    cin>>newUser.username;
    p("Password:");
    cin>>newUser.password;
    Inode root(0);
    newUser.inodeAddress=inodeTable.add(&root);
    fstream sysfile(MAINFILE,ios::in|ios::out|ios::binary);
    size_t userNum=0;
    sysfile.read(reinterpret_cast<char *>(&userNum),sizeof(size_t));
    sysfile.seekp(sizeof(size_t)+userNum*sizeof(User));
    sysfile.write(reinterpret_cast<const char*>(&newUser),sizeof(User));
    ++userNum;
    sysfile.seekp(0);
    sysfile.write(reinterpret_cast<const char *>(&userNum),sizeof(size_t));
    sysfile.close();
}

void FileSys::init(){
    prefix+="~ "+string(user.username)+"$ ";
    ifstream sysfile(DATAFILE,ios::in|ios::binary);
    sysfile.seekg(user.inodeAddress);
    sysfile.read(reinterpret_cast<char*>(&rootDir),sizeof(Inode));
    sysfile.close();
    file.setInode(&rootDir);
    fileEntrys=file.getDir();
}
