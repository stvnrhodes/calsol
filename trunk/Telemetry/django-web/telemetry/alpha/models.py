from django.db import models

class Car(models.Model): 
  name = models.CharField(max_length=50)
  token = models.CharField(max_length=15, unique=True)  
  
class DataPacket(models.Model):
  time = models.DateTimeField(auto_now_add=True)
  speed = models.DecimalField(max_digits=5, decimal_places=2, default=0)
  car = models.ForeignKey(Car)
  power = models.FloatField()
  battery_volt = models.FloatField()
  lat = models.FloatField()
  lng = models.FloatField()
