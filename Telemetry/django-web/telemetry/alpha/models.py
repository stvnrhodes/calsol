from django.db import models

class Car(models.Model): 
  name = models.CharField(max_length=50)
  token = models.CharField(max_length=15, unique=True)  
  
class DataPacket(models.Model):
  time = models.DateTimeField(auto_now_add=True)
  speed = models.DecimalField(max_digits=5, decimal_places=2, null=True, blank=True)
  car = models.ForeignKey(Car)
  power = models.FloatField(blank=True, null=True)
  battery_volt = models.FloatField(blank=True, null=True)
  lat = models.FloatField(blank=True, null=True)
  lng = models.FloatField(blank=True, null=True)
  def speed_as_mph(self):
    return float(self.speed) * 2.23693629
