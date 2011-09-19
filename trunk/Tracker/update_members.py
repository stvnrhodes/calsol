# Copyright 2010 by Ka-Ping Yee
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Periodic location update task, run according to cron.yaml."""

__author__ = 'Ka-Ping Yee <kpy@google.com>'

from google.appengine.api.labs import taskqueue
from google.appengine.ext import db
import datetime
import logging
import model
import utils

# A db.Key guaranteed to come before all others.
MIN_KEY = str(db.Key.from_path('\x00', 1))


class UpdateMembers(utils.Handler):
    def get(self):
        # Scan through members, updating each one.  Each request picks up
        # where the last one left off, and processes the next 100 members.
        last_key = db.Key(self.request.get('last_key', MIN_KEY))
        batch = model.Member.all().filter('__key__ >', last_key).fetch(100)
        now = datetime.datetime.utcnow()
        for member in batch:
            member = member.clean(now)
            if member.tags:
                loc = utils.get_location(member)
                l = model.Location(location=loc, user=member.user,
                    location_time=now, tags=member.tags)
                l.put()
                member.set_location(utils.get_location(member), now)
                logging.info('updated member: ' + member.user.email())
        if batch:
            # Schedule a task to continue processing the next batch.
            taskqueue.add(method='GET', url='/_update_members',
                          params={'last_key': str(batch[-1].key())})


if __name__ == '__main__':
    utils.run([('/_update_members', UpdateMembers)], debug=True)
