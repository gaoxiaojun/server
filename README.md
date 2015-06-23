## Copyright (C) 2015 Lukasz Czerwinski

## server
C++ boost::asio based async client/server

## website
https://github.com/wo3kie/server

## license
For license please refer to LICENSE file.

## requirements
C++11  
g++ / clang++  
boost  
OpenSSL (optionally)

## how to build it
make  
or  
make SSL=1

## how to run it
Start any server you wish  
'./echo port'  
'./day_time port'  
'./chat port'  
and client  
'./client port'.  
All of them require port number a an argument.

## examples
Please find  
* echo.cpp
* day_time.cpp
* chat.cpp  
files as example of different styles of servers

## tutorial
For more information check tutorial.txt file.

## .tpp file extension in vim
Insert line below into ~/.vimrc  
`autocmd BufNewFile,BufReadPost *.ino,*.pde set filetype=cpp`

