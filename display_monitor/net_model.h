// 头文件保护宏，防止重复包含
#pragma once

#include <vector>                 // C++标准向量容器
#include "monitor_inter.h"        // 监控基础模型接口
#include "monitor_info.grpc.pb.h" // gRPC生成代码
#include "monitor_info.pb.h"      // Protobuf生成代码

namespace monitor
{
    /**
     * @brief 网络监控数据模型类
     *
     * 继承自MonitorInterModel，负责将网络接口监控数据适配到Qt表格模型中
     * 显示各网络接口的发送/接收速率和数据包统计信息
     */
    class NetModel : public MonitorInterModel
    {
    private:

    public:
        /**
         * @brief 构造函数
         * @param parent 父对象指针，用于Qt对象树管理
         */
        explicit NetModel(QObject* parent = nullptr) : MonitorInterModel(parent)
        {
            // 初始化表格列标题（5列）
            header_ << tr("name");               // 网络接口名称
            header_ << tr("send_rate");          // 发送速率（KB/s）
            header_ << tr("rcv_rate");           // 接收速率（KB/s）
            header_ << tr("send_packets_rate");  // 发送数据包速率（包/秒）
            header_ << tr("rcv_packets_rate");   // 接收数据包速率（包/秒）
        }

        /// @brief 虚析构函数，确保正确释放资源
        virtual ~NetModel() {}

        /**
         * @brief 获取模型行数
         * @param parent 父模型索引，表格模型中通常忽略
         * @return 数据行数（每个网络接口对应一行）
         */
        int rowCount(const QModelIndex& parent = QModelIndex()) const override
        {
            return monitor_data_.size();
        }

        /**
         * @brief 获取模型列数
         * @param parent 父模型索引
         * @return 固定列数（由COLUMN_MAX枚举定义）
         */
        int columnCount(const QModelIndex& parent = QModelIndex()) const override
        {
            return COLUMN_MAX;
        }

        /**
         * @brief 获取表头数据
         * @param section 列索引
         * @param orientation 表头方向
         * @param role 数据角色
         * @return 表头显示内容
         */
        QVariant headerData(int section, Qt::Orientation orientation, int role) const override
        {
            // 只处理水平表头的显示角色
            if (role == Qt::DisplayRole && orientation == Qt::Horizontal)
            {
                return header_[section];
            }

            // 其他情况委托给父类处理（如字体、颜色等样式）
            return MonitorInterModel::headerData(section, orientation, role);
        }

        /**
         * @brief 获取单元格数据
         * @param index 单元格索引
         * @param role 数据角色
         * @return 单元格显示数据
         */
        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override
        {
            // 边界检查：确保列索引有效
            if (index.column() < 0 || index.column() >= COLUMN_MAX)
            {
                return QVariant();  // 返回空值表示无效索引
            }

            // 仅处理显示角色
            if (role == Qt::DisplayRole)
            {
                // 双重边界检查确保数据访问安全
                if (index.row() < monitor_data_.size() && index.column() < COLUMN_MAX)
                {
                    return monitor_data_[index.row()][index.column()];
                }
            }
            return QVariant();
        }

        /**
         * @brief 更新监控信息（核心方法）
         * @param monitor_info Protobuf格式的监控数据
         *
         * 该方法将Protobuf网络数据转换为模型内部格式，并通知视图更新
         * 每个网络接口的数据对应表格中的一行
         */
        void UpdateMonitorInfo(const monitor::proto::MonitorInfo& monitor_info)
        {
            // 通知视图开始模型重置
            beginResetModel();

            // 清空现有数据
            monitor_data_.clear();

            // 遍历所有网络接口数据
            for (int i = 0; i < monitor_info.net_info_size(); i++)
            {
                // 转换并添加每个网络接口的数据
                monitor_data_.push_back(insert_one_net_info(monitor_info.net_info(i)));
            }

            // 通知视图重置完成
            endResetModel();

            return;
        }

    signals:
        /**
         * @brief 数据变化信号
         * @param topLeft 变化区域左上角索引
         * @param bottomRight 变化区域右下角索引
         * @param roles 变化的角色列表
         *
         * 当模型数据发生变化时发射，通知连接的视图更新显示
         */
        void dataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight,
            const QVector<int>& roles);

    private:
        /**
         * @brief 转换单个网络接口的数据
         * @param net_info Protobuf的NetInfo消息
         * @return 包含5个字段的QVariant向量
         *
         * 辅助方法，将Protobuf消息转换为模型内部数据结构
         * 处理网络接口名称和流量统计信息
         */
        std::vector<QVariant> insert_one_net_info(const monitor::proto::NetInfo& net_info)
        {
            std::vector<QVariant> net_info_list;

            // 遍历所有列，按枚举顺序处理数据
            for (int i = NetModelInfo::NAME; i < NetModelInfo::COLUMN_MAX; i++)
            {
                switch (i)
                {
                    case NetModelInfo::NAME:
                        // 网络接口名称需要转换为QString
                        net_info_list.push_back(
                            QString::fromStdString(net_info.name()));
                        break;
                    case NetModelInfo::SEND_RATE:
                        // 发送速率（KB/s）
                        net_info_list.push_back(QVariant(net_info.send_rate()));
                        break;
                    case NetModelInfo::RCV_RATE:
                        // 接收速率（KB/s）
                        net_info_list.push_back(QVariant(net_info.rcv_rate()));
                        break;
                    case NetModelInfo::SEND_PACKETS_RATE:
                        // 发送数据包速率（包/秒）
                        net_info_list.push_back(QVariant(net_info.send_packets_rate()));
                        break;
                    case NetModelInfo::RCV_PACKETS_RATE:
                        // 接收数据包速率（包/秒）
                        net_info_list.push_back(QVariant(net_info.rcv_packets_rate()));
                        break;
                    default:
                        break;  // 枚举完整性保证不会执行到此处
                }
            }
            return net_info_list;
        }

        /// @brief 数据存储容器：二维向量，外层为行（每个网络接口），内层为列（5个字段）
        std::vector<std::vector<QVariant>> monitor_data_;

        /// @brief 表头字符串列表，支持国际化，包含5个列标题
        QStringList header_;

        /**
         * @brief 网络模型信息列枚举
         *
         * 使用枚举代替硬编码数字，提高代码可读性和可维护性
         * COLUMN_MAX作为哨兵值表示总列数
         */
        enum NetModelInfo
        {
            NAME = 0,                 ///< 网络接口名称列
            SEND_RATE,                ///< 发送速率列（KB/s）
            RCV_RATE,                 ///< 接收速率列（KB/s）
            SEND_PACKETS_RATE,        ///< 发送数据包速率列（包/秒）
            RCV_PACKETS_RATE,         ///< 接收数据包速率列（包/秒）
            COLUMN_MAX                ///< 列总数，用于循环终止条件
        };
    };
}  // namespace monitor
