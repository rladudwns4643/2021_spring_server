myid = -1

function set_uid(x)
	myid = x
end

function event_player_move(p_id)
	if(API_get_x(p_id) == API_get_x(myid)) then
		if API_get_y(p_id) == API_get_y(myid) then
			API_SendMessage(myid, player, "!");
		end
	end
end