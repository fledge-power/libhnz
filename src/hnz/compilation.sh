# g++ server2.cpp VoieHNZ.cpp TcpConnexion.cpp hnz_automate.cpp ../hal/srv/CriticalSection.cpp ../hal/srv/automate.cpp ../hal/srv/UtilSitric.cpp hnz_server.cpp  -o server -pthread -std=c++0x
# g++ client.cpp VoieHNZ.cpp TcpConnexion.cpp hnz_automate.cpp ../hal/srv/CriticalSection.cpp ../hal/srv/automate.cpp ../hal/srv/UtilSitric.cpp hnz_client.cpp -o client -pthread -std=c++0x


g++ -c -pthread -std=c++0x -fPIC VoieHNZ.cpp TcpConnexion.cpp ../hal/srv/CriticalSection.cpp ../hal/srv/automate.cpp ../hal/srv/UtilSitric.cpp hnz_server.cpp hnz_client.cpp
# shellcheck disable=SC2035
g++ -shared -fPIC -o liblibhnz.so *.o

g++ client.cpp -o client -pthread -std=c++0x ./liblibhnz.so -lm
g++ server2.cpp -o server2 -pthread -std=c++0x ./liblibhnz.so -lm

rm *.o