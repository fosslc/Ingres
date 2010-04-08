set l=0
set ll=0
while ($l<2)
	if ($ll == 0) then
		unsetenv SEPPARAM_CREATE
		set ll=1
	else
		setenv SEPPARAM_CREATE TRUE
		set ll=0
	endif

	set k=0
	set kk=0
	while ($k<2)
		if ($kk == 0) then
			unsetenv SEPPARAM_SILENT
			set kk=1
		else
			setenv SEPPARAM_SILENT TRUE
			set kk=0
		endif
	
		set j=0
		set jj=0
		while ($j<2)
			if ($jj == 0) then
				unsetenv SEPPARAM_AUTO
				set jj=1
			else
				setenv SEPPARAM_AUTO TRUE
				set jj=0
			endif
		
			set i=0
			set ii=0
			while ($i<2)
				if ($ii == 0) then
					unsetenv SEPPARAM_DJ
					set ii=1
				else
					setenv SEPPARAM_DJ TRUE
					set ii=0
				endif
		
				qasetuser testenv sep zaa10.sep
		
				if ($i == 0) then
					set i=1
				else
					set i=2
				endif
			end
		
			if ($j == 0) then
				set j=1
			else
				set j=2
			endif
		end
	
		if ($k == 0) then
			set k=1
		else
			set k=2
		endif
	end

	if ($l == 0) then
		set l=1
	else
		set l=2
	endif
end
