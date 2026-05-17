# ecampus 

# Build from source

create file `.env` with postgres connection info, for example:
```
DB_CONNECTION_SECRET='host=localhost port=5432 dbname=test user=test password=test'
```

Run:

```sh
sudo apt-get update
sudo apt-get install build-essential apache2 libcgicc-dev clang libpqxx-dev libboost-json-dev libcrypt-dev libboost-log-dev libboost-test-dev
make 
sudo make install 
```
