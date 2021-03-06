function d = setCoverageEqs(d,onoff)
% SETCOVERAGEEQS - Enable or disable solving the coverage equations.
%
if d.domain_type ~= 6
    error('Wrong domain type. Expected a reacting surface domain.')
end

ion = -1;
if isa(onoff,'char')
    if strcmp(onoff,'on') || strcmp(onoff,'yes')
        ion = 1;
    elseif strcmp(onoff,'off') || strcmp(onoff,'no')
        ion = 0;
    else
        error(strcat('unknown option: ',onoff))
    end
elseif isa(onoff,'numeric')
    ion = onoff;
end
domain_methods(d.dom_id, 120, ion);
