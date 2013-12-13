// -*- tab-width: 4; Mode: C++; c-basic-offset: 4; indent-tabs-mode: nil -*-

// ***************************************
// 関数群
// ***************************************

// ターゲット位置を動かしloiterコントローラを呼ぶ
static void update_chaser() {
	static uint32_t last = 0;		// 前回この関数を呼び出した時刻[ms]
	Vector3f target_distance_last;		// 前回のorigin基準のターゲット距離[cm]
	
	// 前回からの経過時間を計算する
	uint32_t now = hal.scheduler->millis();
	float dt = (now - last)/1000.0f;
	last = now;
	
	// CHASERモードの準備ができていない場合はloiterっぽい状態にする
	// loiterコントローラを呼ぶのみで終了
	if (!chaser_started) {
		wp_nav.update_loiter();
		return;
	}
	
	if (dt > 0.0f) {		// 0割防止
		// chaser_targetを計算
		target_distance_last = target_distance;
		target_distance.x = target_distance.x + chaser_target_vel.x * dt;
		target_distance.y = target_distance.y + chaser_target_vel.y * dt;
		target_distance.z = 0;
		chaser_target.x = chaser_origin.x + target_distance.x;
		chaser_target.y = chaser_origin.y + target_distance.y;
		chaser_target.z = CHASER_ALT;
		
		// chaser_targetがchaser_destinationを越えている場合、目標速度を0とする
		if (fabsf(target_distance.x) >= fabsf(chaser_track_length.x)) {
			chaser_dest_vel.x = 0;
		}
		if (fabsf(target_distance.y) >= fabsf(chaser_track_length.y)) {
			chaser_dest_vel.y = 0;
		}
		
		// chaser_target_velを計算
		chaser_target_vel.x = (target_distance.x - target_distance_last.x) / dt;
		chaser_target_vel.y = (target_distance.y - target_distance_last.y) / dt;
		chaser_target_vel.z = 0;
		
		// chaser_target_velを加減速
		chaser_target_vel.x = constrain_float(chaser_dest_vel.x, chaser_target_vel.x - CHASER_TARGET_ACCEL * dt, chaser_target_vel.x + CHASER_TARGET_ACCEL * dt);
		chaser_target_vel.y = constrain_float(chaser_dest_vel.y, chaser_target_vel.y - CHASER_TARGET_ACCEL * dt, chaser_target_vel.y + CHASER_TARGET_ACCEL * dt);
		
		// yawの目標値を計算し更新する
		chaser_yaw_target = calc_chaser_yaw_target(chaser_target);
		
		// loiterターゲット位置更新
		wp_nav.set_loiter_target(chaser_target);
	}
	// loiterコントローラを呼ぶ
	wp_nav.update_loiter_for_chaser();
}

static void update_chaser_origin_destination(const Vector3f beacon_loc, const Vector3f beacon_loc_last, float dt) {
	// 起点を現在のターゲット位置にする
	chaser_origin = chaser_target;
	
	// beaconの到達予測位置をdestinationとする
	chaser_destination.x = 2*beacon_loc.x - beacon_loc_last.x;
	chaser_destination.y = 2*beacon_loc.y - beacon_loc_last.y;
	chaser_destination.z = CHASER_ALT;
	
	// track関連の計算
	chaser_track_length.x = chaser_destination.x - chaser_origin.x;
	chaser_track_length.y = chaser_destination.y - chaser_origin.y
	chaser_track_length.z = 0;
	
	// target_distanceを0にする
	target_distance.zero();
	
	// 目標速度計算
	// 初回は0とする。またchaser_target_velも0にする
	if (started) {
		chaser_dest_vel.x = constrain_float(chaser_track_length.x / dt, -CHASER_TARGET_VEL_MAX, CHASER_TARGET_VEL_MAX);
		chaser_dest_vel.y = constrain_float(chaser_track_length.y / dt, -CHASER_TARGET_VEL_MAX, CHASER_TARGET_VEL_MAX);
		chaser_dest_vel.z = 0;
	} else {
		chaser_dest_vel.zero();
		
		// 初回のみchaser_target_velを0にする
		chaser_target_vel.zero();
	}
	
}

// 現在値からtargetへので目標角度を計算する
// input: target(homeからの距離[cm])
// output: 現在地からtargetへの角度(-1800〜+1800) [centi-degrees]
static int32_t calc_chaser_yaw_target(const Vector3f target){
	return (int32_t)pv_get_bearing_cd(inertial_nav.get_position(), target);
}
