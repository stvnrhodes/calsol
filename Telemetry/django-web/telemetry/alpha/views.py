from alpha.models import Car
from alpha.models import DataPacket
from django.shortcuts import render_to_response
from django.http import HttpResponse
import json

class PostError(Exception):
  def __init__(self, msg=None):
    self.msg = msg

def random_token(N):
  return ''.join(random.choice(string.ascii_uppercase + string.digits)
          for x in range(N))

def cars(request):
  response = {}
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
  
# Post a data packet, either from a device or a test interface
def post(request):
  response = {}
  try:
    if request.method != 'POST':
      raise PostError('Expect %s method, got %s as method.' % request.method)
    if 'id' not in request.POST:
      raise PostError('Missing expected parameter: id')
    if 'token' not in request.POST:
      raise PostError('Missing expected parameter: token')
    car_id = request.POST['id']
    token = request.POST['token']
    try:
      car = Car.objects.get(pk=car_id, token=token)
    except DoesNotExist:
      raise PostError('Invalid id or token')
    packet = DataPacket(car=car)
    packet.speed = request.POST['speed'] or 0
    packet.power = request.POST['power'] or 0
    packet.battery_volt = request.POST['battery_volt'] or 0
    packet.save()
  except PostError(e):
    response['success'] = 'false'
    response['error'] = e.msg
  else:
    response['success'] = 'true'
    response['packet_id'] = packet.pk
  finally:
    return HttpResponse(json.dumps(response))
