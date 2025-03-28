/* Copyright (c) 2008-2024 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */
#pragma once

#include <QtNetwork>
#include <qlocalsocket.h>

#include "types.h"

class QLocalSocket;

namespace MR::GUI::MRView::Sync {
/**
 * Auto reads data from its local socket when data arrives, and fires an event with that data attached
 */
class LocalSocketReader : public QObject {
  Q_OBJECT

public:
  LocalSocketReader(QLocalSocket *mySocket);

signals:
  void DataReceived(std::vector<std::shared_ptr<QByteArray>> dat); // emits every message currently available

private slots:
  void OnDataReceived();

private:
  QLocalSocket *socket;
};

} // namespace MR::GUI::MRView::Sync
