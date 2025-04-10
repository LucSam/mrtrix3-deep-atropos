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

#include "mrview/sync/enums.h"
#include "mrview/sync/interprocesscommunicator.h"

namespace MR::GUI::MRView::Sync {
/**
 * Syncs values from mrview's window, using the interprocess syncer. In a diagram:
 * _____________Process 1______________    _______________Process 2_____________
 * |                                   |   |                                    |
 * | window <--> SyncManager <--> IPC  <===> IPC <---> SyncManager <---> window |
 * |___________________________________|   |____________________________________|
 *
 * IPC=InterprocessCommunicator
 */
class SyncManager : public QObject {
  Q_OBJECT

public:
  SyncManager();
  bool GetInErrorState();

private slots:
  void OnWindowFocusChanged();
  void OnIPSDataReceived(std::vector<std::shared_ptr<QByteArray>> all_messages);

private:
  InterprocessCommunicator *ips;                                       // used to communicate with other processes
  QByteArray ToQByteArray(Eigen::Vector3f data);                       // conversion utility
  Eigen::Vector3f FromQByteArray(QByteArray vec, unsigned int offset); // conversion utility
  bool SendData(DataKey code, QByteArray data);                        // sends data to other processes via the ips
};

} // namespace MR::GUI::MRView::Sync
