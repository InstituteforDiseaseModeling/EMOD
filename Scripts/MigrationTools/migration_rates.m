city_names = { 'Namawala1', 'Namawala2' };
Population = [ 5 5 ]; % in thousands (IMPORT will be 10 individuals
N = length(city_names);

fromRow_toCol = zeros(N,N);

% Goes <--> Rotterdam
fromRow_toCol(1,2) = 0.003; % number of migrating people per DAY in thousands
fromRow_toCol(2,1) = 0.00001;


migVec = [  kron( (1:N)', ones(N,1) ) ...
            kron( ones(N,1), (1:N)' ) ...
            reshape( fromRow_toCol' * diag(1./Population), N^2, 1)       ];
