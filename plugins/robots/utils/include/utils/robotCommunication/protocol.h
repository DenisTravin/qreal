/* Copyright 2015 Yurii Litvinov
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License. */

#pragma once

#include <QtCore/QObject>

namespace utils {
namespace robotCommunication {

/// Abstract communication protocol. Represents sequential communication which has a possibility of
/// failure and consists of several elementary actions like sendings and receivings of messages. It is responsible
/// only for correct order and overall sanity of communications, actual sending and receiving of data left to
/// concrete actions.
///
/// It automatically runs actions using QThreadPool threads, so it does not block calling thread.
class Protocol : public QObject
{
	Q_OBJECT

public:
};

}
}