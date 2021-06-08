myid = -1
num = 0
meet_player_id = -1

local clock = os.clock
function sleep(n)  -- seconds
	local t0 = clock()
	while clock() - t0 <= n do 
	end
end

function set_uid(x)
	myid = x
	num = 0
end

function event_meet_player(player_id)
	meet_player_id = player_id
	meet_x = API_get_x(meet_player_id)
	meet_y = API_get_y(meet_player_id)
	my_x = API_get_x(my_id)
	my_y = API_get_y(my_id)
	if meet_x == my_x then
		if meet_y ==my_y then
			API_send_chat_packet(meet_player_id, my_id, "HELLO")
			API_add_npc_move_event(my_id)
			sleep(1);
			API_add_npc_move_event(my_id)
			sleep(1);
			API_add_npc_move_event(my_id)
			sleep(1);
			API_send_chat_packet(meet_player_id, my_id, "BYE")
		end
	end
end