var CHART_API = 'http://chart.apis.google.com/chart?';
var SELF_PIN = new google.maps.MarkerImage(
    CHART_API + 'chst=d_map_pin_letter&chld=|f77|0',
    new google.maps.Size(40, 37),
    new google.maps.Point(0, 0),
    new google.maps.Point(9, 35)
);
var HIGHLIGHTED_SELF_PIN = new google.maps.MarkerImage(
    CHART_API + 'chst=d_map_pin_letter&chld=|ff0|0',
    new google.maps.Size(40, 37),
    new google.maps.Point(0, 0),
    new google.maps.Point(9, 35)
);
var UNHIGHLIGHTED_PIN = new google.maps.MarkerImage(
    CHART_API + 'chst=d_map_spin&chld=0.4|0|faa|0|',
    new google.maps.Size(17, 28),
    new google.maps.Point(0, 0),
    new google.maps.Point(8, 28)
);
var HIGHLIGHTED_PIN = new google.maps.MarkerImage(
    CHART_API + 'chst=d_map_spin&chld=0.4|0|ff0|0|',
    new google.maps.Size(17, 28),
    new google.maps.Point(0, 0),
    new google.maps.Point(8, 28)
);

var markers = [];
var rows = [];

function $(id) {
  return document.getElementById(id);
}

function initialize_map(element) {
  return new google.maps.Map(element, {
    zoom: 10,
    center: new google.maps.LatLng(0, 0),
    mapTypeId: google.maps.MapTypeId.ROADMAP,
    mapTypeControl: true,
    mapTypeControlOptions:
        {style: is_mobile ? google.maps.MapTypeControlStyle.DROPDOWN_MENU :
                            google.maps.MapTypeControlStyle.DEFAULT},
    scaleControl: true,
    navigationControl: true,
    navigationControlOptions:
        {style: google.maps.NavigationControlStyle.DEFAULT}
  });
}

function create_markers(map, members) {
  var markers = [];
  for (var i = 0; i < members.length; i++) {
    var marker = new google.maps.Marker({
      map: map,
      icon: members[i].is_self ? SELF_PIN : UNHIGHLIGHTED_PIN,
      position: new google.maps.LatLng(members[i].lat, members[i].lon),
      title: members[i].nickname
    });
    google.maps.event.addListener(marker, 'mouseover', make_highlighter(i));
    google.maps.event.addListener(marker, 'mouseout', make_unhighlighter(i));
    markers.push(marker);
  }
  return markers;
}

function fit_map_bounds(members) {
  var bounds = new google.maps.LatLngBounds();
  for (var i = 0; i < members.length; i++) {
    bounds.extend(new google.maps.LatLng(members[i].lat, members[i].lon));
  }
  map.fitBounds(bounds);
}

function populate_list(tbody, members, show_distance) {
  var rows = [];
  var option = document.createElement('option');
  option.innerText =
      members.length + ' member' + (members.length == 1 ? '' : 's');
  option.value = '';
  $('member-select').appendChild(option);
  for (var i = 0; i < members.length; i++) {
    // Add a row to the member table.
    var tr = document.createElement('tr');
    tr.onclick = make_selector(i);
    var td = document.createElement('td');
    td.className = 'nickname';
    td.innerText = members[i].nickname;
    td.onmouseover = make_highlighter(i);
    td.onmouseout = make_unhighlighter(i);
    tr.appendChild(td);
    var distance = '';
    if (show_distance) {
      var km = Math.round(members[i].distance / 100)/10 + '';
      if (km.indexOf('.') < 0) {
        km += '.0';
      }
      if (members[i].distance >= 10000) {
        km = Math.round(members[i].distance / 1000);
      }
      distance = members[i].is_self ? '(you)' : km + ' km';

      // Add a cell for the distance.
      var td = document.createElement('td');
      td.className = 'distance';
      td.innerText = distance;
      td.onmouseover = make_highlighter(i);
      td.onmouseout = make_unhighlighter(i);
      tr.appendChild(td);
    }
    tbody.appendChild(tr);
    rows.push(tr);

    // Add an option to the member menu.
    option = document.createElement('option');
    option.innerText = members[i].nickname + ' ' + distance;
    option.value = i;
    option.onclick = make_selector(i);
    $('member-select').appendChild(option);
  }
  if (members.length == 0) {
    var tr = document.createElement('tr');
    var td = document.createElement('td');
    if (show_distance) {
      td.colSpan = 2;
    }
    td.innerText = 'No one here.  You could be first!';
    tr.appendChild(td);
    tbody.appendChild(tr);
  }
  return rows;
}

function select(i) {
  if (i !== '') {
    map.panTo(new google.maps.LatLng(members[i].lat, members[i].lon));
    highlight(i);
    // Make the pushpin flash twice, then revert the dropdown selection.
    setTimeout(function() { unhighlight(i); }, 600);
    setTimeout(function() { highlight(i); }, 1000);
    setTimeout(function() { unhighlight(i); }, 1400);
    setTimeout(function() { $('member-select').value = ''; }, 1800);
  }
}

function make_selector(i) {
  return function() { select(i); };
}

function highlight(i) {
  rows[i].style.backgroundColor = '#ff8';
  markers[i].setIcon(
    members[i].is_self ? HIGHLIGHTED_SELF_PIN : HIGHLIGHTED_PIN);
}

function make_highlighter(i) {
  return function() { highlight(i); };
}

function unhighlight(i) {
  rows[i].style.backgroundColor = '#fff';
  markers[i].setIcon(members[i].is_self ? SELF_PIN : UNHIGHLIGHTED_PIN);
}

function make_unhighlighter(i) {
  return function() { unhighlight(i); };
}
