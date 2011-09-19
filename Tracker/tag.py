# Copyright 2010 Google Inc.
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

"""Tag viewing page for the Latitude Tags application."""

__author__ = 'Ka-Ping Yee <kpy@google.com>'

from google.appengine.api.labs import taskqueue
import datetime
import geo
import model
import simplejson
import utils


class Tag(utils.Handler):
    def get(self, tag):
        # To allow a join action to redirect to OAuth and smoothly redirect
        # back here to finish the action, the join action is a GET request.
        if self.request.get('join'):
            # Add a tag to the member, and also update the member's location.
            self.require_member()
            self.verify_signature()
            now = datetime.datetime.utcnow()
            duration = int(self.request.get('duration'))
            stop_time = now + datetime.timedelta(0, duration)
            self.member.set_location(utils.get_location(self.member), now)
            model.Member.join(self.user, tag, stop_time)
            # Ensure that a TagStat entity exists, to be later updated.
            model.TagStat.get_or_insert(key_name=tag)
            # Schedule a task to update the stats now.
            taskqueue.add(
                method='GET', url='/_update_tagstats', params={'tag': tag})
            # Schedule a task to promptly update the stats upon expiry.
            taskqueue.add(
                method='GET', countdown=duration + 1,
                url='/_update_tagstats', params={'tag': tag})
            # Redirect to avoid adding the join action to the browser history.
            raise utils.Redirect('/' + tag)

        # Generate the tag viewing page.
        now = datetime.datetime.utcnow()
        user_id = self.user and self.user.user_id()
        join_time = ''
        members = []
        for member in model.Member.get_for_tag(tag, now):
            if member.user.user_id() == user_id:
                join_time = utils.describe_delta(
                    member.get_stop_time(tag) - now)
            else:
                members.append(self.get_member_info(member))
        members.sort(key=lambda m: (m.get('distance', 0), m['nickname']))
        if join_time:
            members.insert(0, self.get_member_info(self.member))
            members[0]['is_self'] = 1
        if self.user:
            self.set_signature()  # to prevent XSRF
        self.render('templates/tag.html', vars=self.vars, tag=tag,
                    join_time=join_time, members=simplejson.dumps(members))

    def get_member_info(self, member):
        info = {'nickname': member.nickname,
                'lat': member.location.lat,
                'lon': member.location.lon}
        if self.member:
            info['distance'] = geo.distance(
                self.member.location, member.location)
        return info


if __name__ == '__main__':
    utils.run([('/([a-z0-9]+)', Tag)], debug=True)
