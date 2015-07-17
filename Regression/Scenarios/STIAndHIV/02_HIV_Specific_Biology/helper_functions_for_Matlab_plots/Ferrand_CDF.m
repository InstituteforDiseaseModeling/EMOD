% SUMMARY: Simple function to evaluate the cumulative distribution of the Ferrand survival model.
% INPUT:
%   alpha: fraction receiving exponential survival
%   beta: parameter of expoenential
%   mu: scale of weibull
%   s: shape of weibull
% OUTPUT: cumulative probability y

function y = Ferrand_CDF(x,alpha, beta, mu, s)

y = alpha*exp(-beta*x) + (1-alpha)*2.^(-(x/mu).^s);

end
