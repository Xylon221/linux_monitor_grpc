// 头文件保护宏，防止重复包含
#pragma once

#include <vector>                 // C++标准向量容器
#include "monitor_inter.h"        // 监控基础模型接口
#include "monitor_info.grpc.pb.h" // gRPC生成代码
#include "monitor_info.pb.h"      // Protobuf生成代码

namespace monitor
{
    /**
     * @brief 内存监控数据模型类
     *
     * 继承自MonitorInterModel，负责将系统内存监控数据适配到Qt表格模型中
     * 显示详细的内存使用情况，包括物理内存、缓存、交换分区等19个关键指标
     */
    class MemModel : public MonitorInterModel
    {
    private:

    public:
        /**
         * @brief 构造函数
         * @param parent 父对象指针，用于Qt对象树管理
         */
        explicit MemModel(QObject* parent = nullptr) : MonitorInterModel(parent)
        {
            // 初始化表格列标题（19列）
            header_ << tr("used_percent");   // 内存使用百分比
            header_ << tr("total");          // 总物理内存
            header_ << tr("free");           // 空闲内存
            header_ << tr("avail");          // 可用内存（包括可回收缓存）
            header_ << tr("buffers");        // 缓冲区内存
            header_ << tr("cached");         // 页面缓存
            header_ << tr("swap_cached");    // 交换缓存
            header_ << tr("active");         // 活跃内存
            header_ << tr("in_active");      // 非活跃内存
            header_ << tr("active_anon");    // 活跃匿名页
            header_ << tr("inactive_anon");  // 非活跃匿名页
            header_ << tr("active_file");    // 活跃文件页
            header_ << tr("inactive_file");  // 非活跃文件页
            header_ << tr("dirty");          // 脏页
            header_ << tr("writeback");      // 回写页
            header_ << tr("anon_pages");     // 匿名页
            header_ << tr("mapped");         // 映射页
            header_ << tr("kReclaimable");   // 内核可回收内存
            header_ << tr("sReclaimable");   // Slab可回收内存
            header_ << tr("sUnreclaim");     // Slab不可回收内存
        }

        /// @brief 虚析构函数，确保正确释放资源
        virtual ~MemModel() {}

        /**
         * @brief 获取模型行数
         * @param parent 父模型索引，表格模型中通常忽略
         * @return 数据行数（内存数据只有1行）
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
         * 该方法将Protobuf内存数据转换为模型内部格式，并通知视图更新
         * 内存数据为系统级信息，只有一行数据
         */
        void UpdateMonitorInfo(const monitor::proto::MonitorInfo& monitor_info)
        {
            // 通知视图开始模型重置
            beginResetModel();

            // 清空现有数据
            monitor_data_.clear();

            // 转换并添加内存数据（只有一行）
            monitor_data_.push_back(insert_one_mem_info(monitor_info.mem_info()));

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
         * @brief 转换内存信息数据
         * @param mem_info Protobuf的MemInfo消息
         * @return 包含19个字段的QVariant向量
         *
         * 辅助方法，将Protobuf消息转换为模型内部数据结构
         * 处理系统内存的详细统计信息
         */
        std::vector<QVariant> insert_one_mem_info(const monitor::proto::MemInfo& mem_info)
        {
            std::vector<QVariant> mem_info_list;

            // 遍历所有列，按枚举顺序处理数据
            for (int i = MemInfo::USED_PERCENT; i < MemInfo::COLUMN_MAX; i++)
            {
                switch (i)
                {
                    case MemInfo::USED_PERCENT:
                        mem_info_list.push_back(QVariant(mem_info.used_percent()));
                        break;
                    case MemInfo::TOTAL:
                        mem_info_list.push_back(QVariant(mem_info.total()));
                        break;
                    case MemInfo::FREE:
                        mem_info_list.push_back(QVariant(mem_info.free()));
                        break;
                    case MemInfo::AVAIL:
                        mem_info_list.push_back(QVariant(mem_info.avail()));
                        break;
                    case MemInfo::BUFFERS:
                        mem_info_list.push_back(QVariant(mem_info.buffers()));
                        break;
                    case MemInfo::CACHED:
                        mem_info_list.push_back(QVariant(mem_info.cached()));
                        break;
                    case MemInfo::SWAP_CACHED:
                        mem_info_list.push_back(QVariant(mem_info.swap_cached()));
                        break;
                    case MemInfo::ACTIVE:
                        mem_info_list.push_back(QVariant(mem_info.active()));
                        break;
                    case MemInfo::INACTIVE:
                        mem_info_list.push_back(QVariant(mem_info.inactive()));
                        break;
                    case MemInfo::ACTIVE_ANON:
                        mem_info_list.push_back(QVariant(mem_info.active_anon()));
                        break;
                    case MemInfo::INACTIVE_ANON:
                        mem_info_list.push_back(QVariant(mem_info.inactive_anon()));
                        break;
                    case MemInfo::DIRTY:
                        mem_info_list.push_back(QVariant(mem_info.dirty()));
                        break;
                    case MemInfo::WRITEBACK:
                        mem_info_list.push_back(QVariant(mem_info.writeback()));
                        break;
                    case MemInfo::ANON_PAGES:
                        mem_info_list.push_back(QVariant(mem_info.anon_pages()));
                        break;
                    case MemInfo::MAPPED:
                        mem_info_list.push_back(QVariant(mem_info.mapped()));
                        break;
                    case MemInfo::KRECLAIMABLE:
                        mem_info_list.push_back(QVariant(mem_info.kreclaimable()));
                        break;
                    case MemInfo::SRECLAIMABLE:
                        mem_info_list.push_back(QVariant(mem_info.sreclaimable()));
                        break;
                    case MemInfo::SUNRECLAIM:
                        mem_info_list.push_back(QVariant(mem_info.sunreclaim()));
                        break;
                    default:
                        break;  // 枚举完整性保证不会执行到此处
                }
            }
            return mem_info_list;
        }

        /// @brief 数据存储容器：二维向量，外层为行（系统内存只有1行），内层为列（19个字段）
        std::vector<std::vector<QVariant>> monitor_data_;

        /// @brief 表头字符串列表，支持国际化，包含19个列标题
        QStringList header_;

        /**
         * @brief 内存信息列枚举
         *
         * 使用枚举代替硬编码数字，提高代码可读性和可维护性
         * COLUMN_MAX作为哨兵值表示总列数
         */
        enum MemInfo
        {
            USED_PERCENT = 0,   ///< 内存使用百分比列
            TOTAL,              ///< 总物理内存列（GB）
            FREE,               ///< 空闲内存列（GB）
            AVAIL,              ///< 可用内存列（GB）
            BUFFERS,            ///< 缓冲区内存列（GB）
            CACHED,             ///< 页面缓存列（GB）
            SWAP_CACHED,        ///< 交换缓存列（GB）
            ACTIVE,             ///< 活跃内存列（GB）
            INACTIVE,           ///< 非活跃内存列（GB）
            ACTIVE_ANON,        ///< 活跃匿名页列（GB）
            INACTIVE_ANON,      ///< 非活跃匿名页列（GB）
            DIRTY,              ///< 脏页列（GB）
            WRITEBACK,          ///< 回写页列（GB）
            ANON_PAGES,         ///< 匿名页列（GB）
            MAPPED,             ///< 映射页列（GB）
            KRECLAIMABLE,       ///< 内核可回收内存列（GB）
            SRECLAIMABLE,       ///< Slab可回收内存列（GB）
            SUNRECLAIM,         ///< Slab不可回收内存列（GB）
            COLUMN_MAX          ///< 列总数，用于循环终止条件
        };
    };
}  // namespace monitor
