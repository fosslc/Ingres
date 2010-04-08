set j=0
set jj=0
while ($j<2)
	if ($jj == 0) then
		unsetenv SEPPARAMIF1
		set jj=1
	else
		setenv SEPPARAMIF1 TRUE
		set jj=0
	endif

	set i=0
	set ii=0
	while ($i<2)
		if ($ii == 0) then
			unsetenv SEPPARAMIF2
			set ii=1
		else
			setenv SEPPARAMIF2 TRUE
			set ii=0
		endif

		sep zaa03.sep

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
