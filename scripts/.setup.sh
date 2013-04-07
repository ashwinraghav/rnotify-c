#install glib
sudo apt-get install libglib2.0-dev
sudo apt-get install uuid
sudo apt-get install uuid-dev

#install git
sudo apt-get install git


#install zeromq
cd ~
wget http://download.zeromq.org/zeromq-3.2.2.tar.gz
tar -xvf zeromq-3.2.2.tar.gz
cd zeromq-3.2.2/
./configure
make
sudo make install
sudo ldconfig
cd ~

#install czmq
wget http://download.zeromq.org/czmq-1.3.2.tar.gz
tar -xvf czmq-1.3.2.tar.gz 
cd czmq-1.3.2/
./configure
make
sudo make install
sudo ldconfig
cd ~


#install lib-hashring
git clone https://github.com/chrismoos/hash-ring.git
cd hash-ring/
cd ~

#clone rnotify repo
git clone https://github.com/ashwinraghav/rnotify-c.git
cd rnotify-c/scripts

#create watchables
sudo mkdir /localtmp/
sudo mkdir /localtmp/dump
sudo chmod -R 777 /localtmp/
cd /localtmp/dump
cp ~/rnotify-c/scripts/create.sh ~/rnotify-c/scripts/modify.sh /localtmp/dump/
bash create.sh
cd ~



