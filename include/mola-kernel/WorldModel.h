/* -------------------------------------------------------------------------
 *   A Modular Optimization framework for Localization and mApping  (MOLA)
 * Copyright (C) 2018-2019 Jose Luis Blanco, University of Almeria
 * See LICENSE for license information.
 * ------------------------------------------------------------------------- */
/**
 * @file   WorldModel.h
 * @brief  The main class for a "map" or "world model".
 * @author Jose Luis Blanco Claraco
 * @date   Nov 26, 2018
 */
#pragma once

#include <mola-kernel/Entity.h>
#include <mola-kernel/Factor.h>
#include <mola-kernel/FastAllocator.h>
#include <mola-kernel/id.h>
#include <mola-kernel/interfaces/ExecutableBase.h>
#include <map>
#include <shared_mutex>

namespace mola
{
/** The main class for a "map" or "world model".
 *
 * \ingroup mola_kernel_grp
 */
class WorldModel : public ExecutableBase
{
    DEFINE_MRPT_OBJECT(WorldModel)

   public:
    // Virtual interface of any ExecutableBase. See base docs:
    void initialize_common(const std::string&) override {}
    void initialize(const std::string& cfg_block) override;
    void spinOnce() override;

    /** The WorldModel is launched first, before most other modules. */
    int launchOrderPriority() const override { return 10; }

    struct Parameters
    {
        double age_to_unload_keyframes{15.0};  //!< [s]
    };

    Parameters params_;

    using entity_connected_factors_t =
        mola::fast_map<id_t, mola::fast_set<fid_t>>;

    /** @name Main API
     * @{ */
    void entities_lock_for_read() { entities_mtx_.lock_shared(); }
    void entities_unlock_for_read() { entities_mtx_.unlock_shared(); }
    void entities_lock_for_write() { entities_mtx_.lock(); }
    void entities_unlock_for_write() { entities_mtx_.unlock(); }

    void factors_lock_for_read() { factors_mtx_.lock_shared(); }
    void factors_unlock_for_read() { factors_mtx_.unlock_shared(); }
    void factors_lock_for_write() { factors_mtx_.lock(); }
    void factors_unlock_for_write() { factors_mtx_.unlock(); }

    const Entity& entity_by_id(const id_t id) const;
    Entity&       entity_by_id(const id_t id);

    const Factor& factor_by_id(const fid_t id) const;
    Factor&       factor_by_id(const fid_t id);

    id_t  entity_emplace_back(Entity&& e);
    fid_t factor_emplace_back(Factor&& f);

    id_t  entity_push_back(const Entity& e);
    fid_t factor_push_back(const Factor& f);

    std::vector<id_t>  entity_all_ids() const;
    std::vector<fid_t> factor_all_ids() const;

    annotations_data_t&       entity_annotations_by_id(const id_t id);
    const annotations_data_t& entity_annotations_by_id(const id_t id) const;

    /** Returns all entities that are connected to a given one by any common
     * factor.
     */
    std::set<id_t> entity_neighbors(const id_t id) const;

    /** @} */

    struct EntitiesContainer;
    struct FactorsContainer;

   private:
    /** All keyframes, relative and absolute poses, calibration parameter sets,
     * etc. that can be stored in a world model.
     * Indexed by a unique id_t; */
    std::unique_ptr<EntitiesContainer> entities_;
    entity_connected_factors_t         entity_connected_factors_;
    std::shared_mutex                  entities_mtx_;

    /** All observations, constraints, etc. as generic "factors".
     * Indexed by a unique fid_t; */
    std::unique_ptr<FactorsContainer> factors_;
    std::shared_mutex                 factors_mtx_;

    mutable mola::fast_map<id_t, mrpt::Clock::time_point> entity_last_access_;
    std::shared_mutex entity_last_access_mtx_;

    /** Returns a list with all those entities that have not been accessed in
     * `age_to_unload_keyframes`. Once an entity is reported as "aged", it's
     * removed from the list of entities to watch, so it will be not reported
     * again unless re-loaded. */
    std::vector<id_t> findEntitiesToSwapOff();

    void internal_update_neighbors(const FactorBase& f);
};

}  // namespace mola
