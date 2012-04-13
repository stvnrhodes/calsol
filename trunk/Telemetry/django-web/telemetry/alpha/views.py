from alpha.models import Car
from alpha.models import DataPacket
from django.shortcuts import render_to_response
from django.views.decorators.csrf import csrf_protect
from django.core.context_processors import csrf
from django.http import HttpResponse
from django.contrib.humanize.templatetags.humanize import naturaltime
from datetime import timedelta
from datetime import datetime
import json
import random
import string

class PostError(Exception):
  def __init__(self, msg=None):
    self.msg = msg

def random_token(N):
  return ''.join(random.choice(string.ascii_uppercase + string.digits + string.ascii_lowercase)
          for x in range(N))

def cars(request):
  response = {}
  response.update(csrf(request))
  response['cars'] = Car.objects.all()
  if request.method == 'POST':
    command = request.POST['command']
    if command == 'create':
      name = request.POST['name']
      new_car = Car(name=name, token=random_token(15))
      new_car.save()
    elif command == 'delete':
      car = Car.objects.get(pk=request.POST['id'])
      car.delete()
  return render_to_response('cars.html', response)
  
def car(request, car_id):
  response = {}
  car = Car.objects.get(pk=car_id)
  response['car'] = car
  response['data_packets'] = DataPacket.objects.filter(car=car).order_by('-time')
  return render_to_response('car.html', response)
  
def cars_json(request): 
  response = {'cars': {}}
  cars = Car.objects.all()
  for car in cars:
    response['cars'][car.pk] = (car.name, car.token)
  return HttpResponse(json.dumps(response))
  
def car_json(request, car_id):
  response = {}
  car = Car.objects.get(pk=car_id)
  data_packets = DataPacket.objects.filter(car=car).order_by('-time')
  if not data_packets:
    return HttpResponse(json.dumps({
      'success': 'true',
      'speed': 0,
      'time': 'Not connected.'
    }))
  first_packet = data_packets[0] or None
  response['speed'] = '%0.1f' % first_packet.speed_as_mph()
  response['time'] = 'Last updated %s' % naturaltime(first_packet.time)
  response['lat'] = first_packet.lat
  response['lng'] = first_packet.lng
  # It is connected if the last packet seen was less than 10 seconds ago
  response['connected'] = (datetime.now() - first_packet.time) < timedelta(0, 10)
  response['success'] = 'true'
  return HttpResponse(json.dumps(response))
  
# Post a data packet, either from a device or a test interface
def post(request):
  response = {}
  try:
    if 'id' not in request.REQUEST:
      raise PostError('Missing expected parameter: id')
    if 'token' not in request.REQUEST:
      raise PostError('Missing expected parameter: token')
    car_id = request.REQUEST['id']
    token = request.REQUEST['token']
    try:
      car = Car.objects.get(pk=car_id, token=token)
    except DoesNotExist:
      raise PostError('Invalid id or token')
    packet = DataPacket(car=car)
    speed = request.REQUEST['speed'] if 'speed' in request.REQUEST else 0
    if speed == 'null' or not speed: speed = 0
    packet.speed = speed
    packet.power = request.REQUEST['power'] if 'power' in request.REQUEST else 0
    packet.battery_volt = request.REQUEST['battery_volt'] if 'battery_volt' in request.REQUEST else 0
    packet.lat = request.REQUEST['lat'] if 'lat' in request.REQUEST else 0
    packet.lng = request.REQUEST['lng'] if 'lng' in request.REQUEST else 0
    packet.save()
  except PostError, e:
    response['success'] = 'false'
    response['error'] = e.msg
  else:
    response['success'] = 'true'
    response['packet_id'] = packet.pk
  return HttpResponse(json.dumps(response))
