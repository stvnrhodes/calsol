# web_connect.py: Connects to a server and dumps telemetry data.

import sqlite3 as sql
from Queue import Queue
import threading
from optparse import OptionParser
import ConfigParser
import urllib
import json
import sys
import os
import time
import datetime


class WebConnect():
  def __init__(self, options, args):
    self.verbose = True
    self.general_options = ConfigParser.RawConfigParser()
    self.general_options.read(os.path.join("config", "general.cfg"))
    try:
      self.host = self.general_options.get('webconnect', 'host')
    except (ConfigParser.NoOptionError, ConfigParser.NoSectionError):
      self.host = None
    self.car_id = None
    self.car_name = None
    self.car_token = None
    self.database = self.general_options.get('logging', 'database')
    self.connection = sql.connect(self.database, 
        detect_types=(sql.PARSE_DECLTYPES | sql.PARSE_COLNAMES))
    self.check_database()
    self.last_checked = datetime.datetime.utcnow()
    
  def check_database(self):
    # Run a quick sanity check to see that the database exists and has the correct tables
    def tableExists(conn, name):
      cur = conn.cursor()
      cur.execute('SELECT name FROM sqlite_master where name = ?;', (name,))
      exists = cur.fetchone() != None
      cur.close()
      return exists
    if (not tableExists(self.connection, 'intervals') 
        or not tableExists(self.connection, 'data')):
      sys.stderr.write('Tables "intervals" and "data" do not exist in database %s, '
                       'aborting\r\n' % self.database)
      sys.exit(1)
      
    
  def get_options(self):
    # Prompt the user to decide where to send the data to
    if not self.host:
      self.host = raw_input('Enter the host server: ')
    
    # Fetch the list of cars
    print 'Fetching list of cars from %s/api/cars' % self.host
    try:
      result = urllib.urlopen('%s/api/cars' % self.host).read()
    except Exception, e:  # TODO: More specific exception
      sys.stderr.write('Error reaching url %s/api/cars\r\n' % self.host)
      sys.stderr.write(str(e))
      sys.exit(1)
    cars = json.loads(result)['cars']
    
    # Select the desired car
    print 'ID\tName\tToken'
    for car_id, (car_name, car_token) in iter(sorted(cars.iteritems())):
      print '%s\t%s\t%s' % (car_id, car_name, car_token)
    self.car_id = raw_input('Select a car ID (or enter to create new): ')
    if self.car_id and self.car_id in cars:
      self.car_name, self.car_token = cars[self.car_id]
    else:
      if not self.car_id:
        self.car_name = raw_input('Enter new car name: ')
      else:
        self.car_name = self.car_id
      print 'Creating new car %s...' % self.car_name
      print '%s/api/cars?command=create&name=%s' % (self.host, self.car_name)
      u = urllib.urlopen('%s/api/cars?command=create&name=%s' % (self.host, self.car_name))
      result = json.loads(u.read())
      if 'new_car' in result:  # Success
        self.car_id = result['new_car']
        self.car_token = result['cars'][str(self.car_id)][1]
        print 'New car "%s" created with id %s and token %s' % (self.car_name, self.car_id, self.car_token)
      else:
        sys.stderr.write('Failed to create new car %s\r\n' % self.car_name)
        sys.exit(1)
  
  
  def run(self):
    if self.verbose:
      self.get_options()
    self.thread = threading.Thread(target=self.runner)
    self.thread.daemon = True
    self.thread.start()
    
    while True:
      cmd = 'SELECT id, name, time, data FROM data WHERE time > ?'
      cursor.execute(cmd, [self.last_checked])
      self.last_checked = datetime.datetime.utcnow()
      for row in cursor:
        id = row['id']
        name = row['name']
        ts = row['time']
        data = row['data']
        print id, name, ts, data
      time.sleep(1)


if __name__ == '__main__':
  parser = OptionParser()

  (options, args) = parser.parse_args()

  app = WebConnect(options, args)
  sys.exit(app.run())  # Should run forever, or until there is an error