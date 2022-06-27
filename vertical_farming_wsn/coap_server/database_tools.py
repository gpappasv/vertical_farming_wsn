import os
from datetime import *
import mysql.connector as database
import time
"""
Connection to measurementsDB database
"""
connection = database.connect(
    user='admin',
    password='adminpass',
    host='localhost',
    database="measurementsDB")

cursor = connection.cursor()

'''
Class to parse row mean data message sent from 9160 and store it in the database
inside the table row_mean_values
'''
class MessageParsing:
    def __init__(self):
        self.msgtype = 0x0
        self.msglen = 0
        self.temperature = 0.0
        self.humidity = 0.0
        self.soilmoisture = 0.0
        self.lightintensity = 0
        self.rowid = 0
        self.timestamp = 0
        self.lightswitch = False
        self.waterswitch = False
        self.fanswitch = False

    @staticmethod
    def insert_into_database(self, id, timestamp, temp, hum, soil, light, lswitch, wswitch, fswitch):
        ts = int(str(int(timestamp/1000)))
        try:
            statement = "INSERT INTO row_mean_values (row_id, timestamp, temperature, humidity, soil_moisture, light_exposure, light_switch, water_switch, fan_switch) VALUES (%s, %s, %s, %s, %s, %s, %s, %s, %s)"
            data = (id, datetime.utcfromtimestamp(ts).strftime('%Y-%m-%d %H:%M:%S'), temp, hum, soil, light, lswitch, wswitch, fswitch)
            cursor.execute(statement, data)
            connection.commit()
        except database.Error as e:
            print(f"Error adding entry to database: {e}")

    def row_mean_data_parsing(self, payload: bytes):
        # Save values on variables after parsing the message
        self.msgtype = payload[0:1]
        self.msglen  = int.from_bytes(payload[1:2], "little")
        self.temperature = int.from_bytes((payload[2:4]), "little") / 100
        self.humidity = int.from_bytes((payload[4:6]), "little") / 100
        self.soilmoisture = int.from_bytes((payload[6:8]), "little") / 100
        self.lightintensity = int.from_bytes((payload[8:10]), "little")
        self.rowid = int.from_bytes((payload[10:11]), "little")
        self.timestamp = int.from_bytes((payload[11:19]), "little")
        self.lightswitch = bool.from_bytes((payload[19:20]), "little")
        self.waterswitch = bool.from_bytes((payload[20:21]), "little")
        self.fanswitch   = bool.from_bytes((payload[21:22]), "little")
        # Write row mean data to database
        self.insert_into_database(self, self.rowid, self.timestamp, self.temperature, self.humidity, self.soilmoisture, self.lightintensity, self.lightswitch, self.waterswitch, self.fanswitch)

class GetDatabaseEntries:
    def __init__(self) -> None:
        self.timestamps = []
        self.temperature = []
        self.light_exposure = []
        self.soil_moisture = []
        self.humidity = []

    # Function to fetch 
    def get_data_from_database(self, row_id, column, list_lenght) -> list:
        # Fetches the most recent entries
        fetch_timestamps_query = "SELECT {attr} FROM row_mean_values WHERE row_id = {id} ORDER BY timestamp DESC LIMIT {num}".format(attr=column, id=row_id, num=list_lenght)
        cursor.execute(fetch_timestamps_query)
        # the result will be list of tuples
        data_list = cursor.fetchall()
        # keep only the first element from the tuples
        data_list = [i[0] for i in data_list]
        # Need to reverse the list as the first element is the most recent and the last the least recent
        data_list.reverse()
        connection.commit()
        return data_list
    
    # Function to fetch the last list_length timestamps from database
    def get_most_recent_timestamps(self, list_length) -> list:
        # Fetches the most recent entries
        fetch_timestamps_query = "SELECT DISTINCT timestamp FROM row_mean_values ORDER BY timestamp DESC LIMIT {num}".format(num=list_length)
        cursor.execute(fetch_timestamps_query)
        # the result will be list of tuples
        data_list = cursor.fetchall()
        # keep only the first element from the tuples
        data_list = [i[0] for i in data_list]
        data_list.reverse()
        for i in range(len(data_list)):
            data_list[i] = data_list[i].strftime("%Y-%m-%d %H:%M:%S")
        connection.commit()
        return data_list

    '''
    Function to create a list of data based on existance or not of entries for each timestamp in the list

    row_id: the row id we care about
    column: the type of data we care about (humidity, temperature etc...)
    timestamps_list: the list of timestamps we care about
    '''
    def create_list_of_data(self, row_id, column, timestamps_list)->list:
        data_list = []
        for index in range(len(timestamps_list)):
            # sql query for single entry
            fetch_timestamps_query = "SELECT {data} FROM row_mean_values WHERE timestamp = '{tstmp}' AND row_id = {id}".format(data=column, tstmp=timestamps_list[index], id=row_id)
            try:
                cursor.execute(fetch_timestamps_query)
                # append the data
                temp_list = cursor.fetchall()
            except database.Error as e:
                print(f"Error adding entry to database: {e}")
            # keep only the first element from the tuples
            temp_list = [i[0] for i in temp_list]
            if len(temp_list) == 0:
                data_list.append('None')
            # Should always be either 0 or 1
            elif len(temp_list) == 1:
                data_list.append(str(temp_list[0]))
            else:
                print("Error in create list of data")
        # Clear temp list just to be safe
        temp_list = []
        connection.commit()
        return data_list

class UserRequestsDBTools:
    def store_threshold_config_request(self, row_id, temp_threshold, hum_threshold, soil_threshold, light_threshold):
        store_threshold_query = "INSERT INTO user_thresholds_request (row_id, timestamp, temperature_threshold, humidity_threshold, soil_moisture_threshold, light_exposure_threshold) VALUES (%s, %s, %s, %s, %s, %s)"
        now = datetime.now()
        data = (row_id, now.strftime("%Y-%m-%d %H:%M:%S"), temp_threshold, hum_threshold, soil_threshold, light_threshold)
        try:
            cursor.execute(store_threshold_query, data)
            connection.commit()
        except database.Error as e:
            print(f"Error adding entry to database: {e}")

    # function that returns the next in order timestamp of the current_timestamp from
    # user_thresholds_request table 
    def get_next_threshold_request_timestamp(self, current_timestamp):
        timestamp_query = "SELECT timestamp FROM user_thresholds_request WHERE timestamp > '{ts}' ORDER BY timestamp ASC LIMIT 1".format(ts=current_timestamp)
        try:
            cursor.execute(timestamp_query)
            timestamp_returned = cursor.fetchall()
            if len(timestamp_returned) == 1:
                timestamp_returned = [i[0] for i in timestamp_returned]
                timestamp_returned = timestamp_returned[0].strftime("%Y-%m-%d %H:%M:%S")
                connection.commit()
                return str(timestamp_returned)
            else:
                connection.commit()
                return ''
        except database.Error as e:
            print(f"Error adding entry to database: {e}")
            return ''
    
    # function in order to know the latest timestamp of user requests. For init purposes only
    def get_latest_timestamp_threshold_request(self):
        timestamp_query = "SELECT timestamp FROM user_thresholds_request ORDER BY timestamp DESC LIMIT 1"
        try:
            cursor.execute(timestamp_query)
            timestamp_returned = cursor.fetchall()
            if len(timestamp_returned) == 1:
                timestamp_returned = [i[0] for i in timestamp_returned]
                timestamp_returned = timestamp_returned[0].strftime("%Y-%m-%d %H:%M:%S")
                connection.commit()
                return str(timestamp_returned)
            else:
                connection.commit()
                return ''
        except database.Error as e:
            print(f"Error adding entry to database: {e}")
            return ''
    
    # function to create the bytearray in order to send the user threshold config request  through coap
    def create_user_threshold_config_message(self, timestamp):
        request_query = "SELECT row_id, temperature_threshold, humidity_threshold, soil_moisture_threshold, light_exposure_threshold  FROM user_thresholds_request WHERE timestamp = '{ts}'".format(ts=timestamp)
        try:
            cursor.execute(request_query)
            data = cursor.fetchall()
            data = [item for t in data for item in t]
            connection.commit()
            message = bytearray([0xB3, 0x0D])
            message += data[0].to_bytes(1, 'little')
            message += data[1].to_bytes(2, 'little')
            message += data[2].to_bytes(2, 'little')
            message += data[4].to_bytes(2, 'little')
            message += data[3].to_bytes(2, 'little')
            # TODO crc
            message += bytearray([0xFF, 0xFF])
            return message
        except database.Error as e:
            print(f"Error adding entry to database: {e}")
            # Random return -> will be discarted
            return bytearray([0xFF, 0x01])

    def store_control_config_request(self, row_id, light_switch, water_switch, fan_switch, automatic_control):
            store_control_query = "INSERT INTO user_control_request (timestamp, row_id, light_switch, water_switch, fan_switch, automatic_control) VALUES (%s, %s, %s, %s, %s, %s)"
            now = datetime.now()
            data = (now.strftime("%Y-%m-%d %H:%M:%S"), row_id, light_switch, water_switch, fan_switch, automatic_control)
            try:
                cursor.execute(store_control_query, data)
                connection.commit()
            except database.Error as e:
                print(f"Error adding entry to database: {e}")

    # function that returns the next in order timestamp of the current_timestamp from
    # user_control_request table 
    def get_next_control_request_timestamp(self, current_timestamp):
        timestamp_query = "SELECT timestamp FROM user_control_request WHERE timestamp > '{ts}' ORDER BY timestamp ASC LIMIT 1".format(ts=current_timestamp)
        try:
            cursor.execute(timestamp_query)
            timestamp_returned = cursor.fetchall()
            if len(timestamp_returned) == 1:
                timestamp_returned = [i[0] for i in timestamp_returned]
                timestamp_returned = timestamp_returned[0].strftime("%Y-%m-%d %H:%M:%S")
                connection.commit()
                return str(timestamp_returned)
            else:
                connection.commit()
                return ''
        except database.Error as e:
            print(f"Error adding entry to database: {e}")
            return ''

    # function in order to know the latest timestamp of user requests. For init purposes only
    def get_latest_timestamp_control_request(self):
        timestamp_query = "SELECT timestamp FROM user_control_request ORDER BY timestamp DESC LIMIT 1"
        try:
            cursor.execute(timestamp_query)
            timestamp_returned = cursor.fetchall()
            if len(timestamp_returned) == 1:
                timestamp_returned = [i[0] for i in timestamp_returned]
                timestamp_returned = timestamp_returned[0].strftime("%Y-%m-%d %H:%M:%S")
                connection.commit()
                return str(timestamp_returned)
            else:
                connection.commit()
                return ''
        except database.Error as e:
            print(f"Error adding entry to database: {e}")
            return ''

    # function to create the bytearray in order to send the user control config request  through coap
    def create_user_control_config_message(self, timestamp):
        request_query = "SELECT row_id, light_switch, water_switch, fan_switch, automatic_control  FROM user_control_request WHERE timestamp = '{ts}'".format(ts=timestamp)
        try:
            cursor.execute(request_query)
            data = cursor.fetchall()
            data = [item for t in data for item in t]
            connection.commit()
            message = bytearray([0xB2, 0x09])
            message += data[0].to_bytes(1, 'little')
            message += data[4].to_bytes(1, 'little')
            message += data[1].to_bytes(1, 'little')
            message += data[2].to_bytes(1, 'little')
            message += data[3].to_bytes(1, 'little')
            # TODO crc
            message += bytearray([0xFF, 0xFF])
            return message
        except database.Error as e:
            print(f"Error adding entry to database: {e}")
            # Random return -> will be discarted
            return bytearray([0xFF, 0x01])