myid = -1;

function set_uid(x)
	myid = x;
end

function player_is_near( p_id )
	p_x = API_get_x(p_id);
	p_y = API_get_y(p_id);
	m_x = API_get_x(myid);
	m_y = API_get_y(myid);

	if(p_x == m_x) then
		if(p_y == m_y) then
			API_send_mess(p_id, my_id, "HELLO");
		end
	end
end
