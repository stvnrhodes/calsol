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

"""Periodic tag statistics update task, run according to cron.yaml."""

__author__ = 'Ka-Ping Yee <kpy@google.com>'

from google.appengine.api.labs import taskqueue
from google.appengine.ext import db
import datetime
import logging
import model
import utils

# A db.Key guaranteed to come before all others.
MIN_KEY = str(db.Key.from_path('\x00', 1))


class UpdateTagStats(utils.Handler):
    def get(self):
        tag = self.request.get('tag', '')
        if tag:
            # Update a single specified tag.
            self.update(tag)
        else:
            # Scan through tags, updating each one.  Each request picks up
            # where the last one left off, and processes the next 5 tags.
            last_key = db.Key(self.request.get('last_key', MIN_KEY))
            batch = model.TagStat.all().filter('__key__ >', last_key).fetch(5)
            for tagstat in batch:
                last_key = str(tagstat.key())
                self.update(tagstat.key().name())
            if batch:
                # Schedule a task to continue processing the next batch.
                taskqueue.add(method='GET', url='/_update_tagstats',
                              params={'last_key': last_key})

    def update(self, tag):
        """Updates the statistics for a single tag, by name."""
        now = datetime.datetime.utcnow()
        tagstat = model.TagStat.get(tag) or model.TagStat(key_name=tag)
        tagstat.member_count = len(model.Member.get_for_tag(tag, now))
        # TODO(kpy): Compute the centroid of the member locations.
        # TODO(kpy): Compute the RMS distance of members from the centroid.
        if tagstat.member_count > 0:
            tagstat.put()
        else:
            tagstat.delete()
        logging.info('updated tag: ' + tag)


if __name__ == '__main__':
    utils.run([('/_update_tagstats', UpdateTagStats)], debug=True)
