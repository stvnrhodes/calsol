# Copyright 2010 Google Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#         http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Main page of the Latitude Tags app."""

__author__ = 'Ka-Ping Yee <kpy@google.com>'

import model
import utils


class Main(utils.Handler):
    def get(self):
        tag = utils.clean_tag(self.request.get('tag', ''))
        if tag:
            raise utils.Redirect('/' + tag)
        tagstats = model.TagStat.all().order('-member_count').fetch(20)
        self.render('templates/main.html', vars=self.vars, tagstats=tagstats)


if __name__ == '__main__':
    utils.run([('/', Main)], debug=True)
