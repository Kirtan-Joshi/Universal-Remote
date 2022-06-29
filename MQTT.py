import random
import time
# import mqtt_client from the paho.mqtt
from paho.mqtt import client as mqtt_client
import pymysql  # import mysql

connected = False
# mqtt broker adrress to publish and subscribe data via broker
broker_address = "broker.mqtt-dashboard.com"

topic = 'SMIT/mqtt'  # the topic defined for the broker
port = 1883
user = "RENESYS" #user and password defined for connect with client
password = "123456"

client_id = 'python-mqtt-1' #defined client id for connection with broker

#mqtt connect to mqtt function
def connect_mqtt(client_id, publisher) -> mqtt_client: #define the client id and publisher for mqtt connect
    def on_connect(client, userdata, flags, rc): #this function will be call back after connecting the client
        if rc == 0:
            print("Publisher successfully Connected to MQTT Broker!") #if return code is 0 then print
        else:
            print("Failed to connect, return code %d\n", rc) #print when return code is not zero

#connecting the client id
    client = mqtt_client.Client(client_id)
    client.username_pw_set(user, password) #client connect to this username and password
    client.on_connect = on_connect #on_connect function
    client.connect(broker_address, port) #client connect to the defined port and broker address
    return client #return the value



#for importing the data from the database created
cnx = pymysql.connect(user="root", passwd="khj913511", host="localhost", database="remote")  #define the mysql user, pwd, host, database
mycursor = cnx.cursor()
mycursor.execute("select * from hexdata") #execute the select hexcode for the given query
result = mycursor.fetchall() #fetch the executed data

#run function for executing the code
def run():
    client = connect_mqtt(client_id, publisher=0) #wether client is connected to broker and publisher
    client.loop_start() #start the loop to check mqtt connection
    time.sleep(2.05) #time delay of 2.05 sec
    mqttQuery = input("Enter required key: ") #input for the query that user want to fetch from mysql database
    res = 0
    for i in result: #for loop to check the input query is in database or not
        if i[0] == mqttQuery: #if the i is same as mqtt query
            print("data is found.") #print if data is found
            res = i[1]
    if not res:
        print('key not found in database.') #print if data is not found
    publisher(client, res)
    client.loop_stop()

#function for publishing the fetched data from mysql database to publisher
def publisher(client, res):
    time.sleep(2.05) #delay of 2.05 sec before the execution
    msg = res
    result = client.publish(topic, msg) #for publishing the msg to declared topic
    print('-------------------------------------------------------------')
    status = result[0]
    if status == 0: #if result is without error
        print(f"data is: `{msg}`") #print the hexcode (msg for the key that is fetched from the database)
        print(f"Send `{msg}` to topic `{topic}`") #send the msg to the declared topic
        time.sleep(2.05)
    else:
        print(f"Failed to send message to topic {topic}") #if data is not found then it will show this msg




run()