# lanzaAsincrono.sh
# Lanza el servidor asincrono de frases que es un daemon y varios clientes
# las ordenes están en un fichero que se pasa como tercer parámetro
./asincrono
./cliente olivo TCP ordenes.txt &
./cliente olivo TCP ordenes1.txt &
./cliente olivo TCP ordenes2.txt &
./cliente olivo UDP ordenes.txt &
./cliente olivo UDP ordenes1.txt &
./cliente olivo UDP ordenes2.txt &

 
