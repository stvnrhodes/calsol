from alpha.models import Car
from alpha.models import DataPacket
from django.shortcuts import render_to_response
from django.views.decorators.csrf import csrf_protect
from django.core.context_processors import csrf
from django.http import HttpResponse
from django.contrib import humanize
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
  
def car_json(request, car_id):
  response = {}
  car = Car.objects.get(pk=car_id)
  data_packets = DataPacket.objects.filter(car=car).order_by('-time')
  if not data_packets:
    return HttpResponse(json.dumps({
      'success': 'true',
      'speed': 0,
      'last_updated': 'Not connected.'
    }))
  first_packet = data_packets[0] or None
  response['speed'] = first_packet.speed
  response['time'] = 'Last updated %s' % humanize.naturaltime(first_packet.time)
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
    packet.speed = request.REQUEST['speed'] or 0
    packet.power = request.REQUEST['power'] or 0
    packet.battery_volt = request.REQUEST['battery_volt'] or 0
    packet.save()
  except PostError(e):
    response['success'] = 'false'
    response['error'] = e.msg
  else:
    response['success'] = 'true'
    response['packet_id'] = packet.pk
  finally:
    return HttpResponse(json.dumps(response))
