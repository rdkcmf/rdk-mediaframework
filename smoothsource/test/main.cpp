/*
* If not stated otherwise in this file or this component's LICENSE file the
* following copyright and licenses apply:
*
* Copyright 2014 RDK Management
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/
#include "../smoothsource.h"
#include "sampvidsink.h"
#include <string>
#include <iostream>
using namespace std;

int main(int argc, char** argv)
{
    bool isRunning = true;

    // Start streaming.
    SmoothSource smoothSource;
    SampVidSink vidSink;

    smoothSource.init();
    smoothSource.open(argv[1], 0);

    vidSink.init();
    vidSink.setSource(&smoothSource);

    smoothSource.play();

    while(isRunning)
    {
        string s = "";
        getline(cin, s);

		if(s.empty())
		{
		}
        else if( s =="quit")
        {
            isRunning = false;
        }
    }

    return 0;
}
