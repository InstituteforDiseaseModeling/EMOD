% least-squares fit to data using Levenberg-Marquardt algorithm

% prepare a figure for plotting
figure(1);clf;set(gcf,'color','w');

% sigmoid function to fit to data
f = @(p,x) ones(size(x))./(1 + exp(-p(1).*(x - p(2))));

% choose maximum number of iterations, tolerance of fit, and
% iteration-level verbosity to record to diary
fit_options = ...
    statset('MaxIter',100000000,'Display','Iter', 'TolFun', 1e-12);


x = [2001
    2002
    2004
    2005
    2006
    2007
    2008
    2009
    2010
    2013];

y = [3%
    5%
    15%
    34%
    52%
    61%
    73%
    83%
    87%
    92]./100;

extra_x_to_plot_beyond_data = 5;

guess = [66,2006];

% use Levenberg-Marquardt to get best-fit parameters
p = nlinfit(x,y,f,guess,fit_options);

% plot best-fit curve
hold on;
plot(x,y,'ko');
x_fit_to_plot = ...
    x(1)-extra_x_to_plot_beyond_data:.1:...
    extra_x_to_plot_beyond_data+x(end);
plot(x_fit_to_plot,f(guess,x_fit_to_plot),'--','color','b','LineWidth',1);
plot(x_fit_to_plot,f(p,x_fit_to_plot),'--','color','r','LineWidth',1);

hold on;

legend('Data','Guess','Best fit','location','southeast')

% plot data on top of best fit curve


% add title to plot
title(['Best-fit parameters: ',num2str(p)]);



